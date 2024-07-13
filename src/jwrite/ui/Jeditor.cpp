#include <jwrite/ui/Jeditor.h>
#include <jwrite/AppConfig.h>
#include <QPainter>

namespace jwrite::ui {

static const QMap<QString, QString> QUOTE_PAIRS{
    {"“", "”"},
    {"‘", "’"},
    {"（", "）"},
    {"【", "】"},
    {"《", "》"},
    {"〔", "〕"},
    {"〈", "〉"},
    {"「", "」"},
    {"『", "』"},
    {"〖", "〗"},
    {"［", "］"},
    {"｛", "｝"},
};

static const QMap<QString, QString> REVERSE_QUOTE_PAIRS{
    {"”", "“"},
    {"’", "‘"},
    {"）", "（"},
    {"】", "【"},
    {"》", "《"},
    {"〕", "〔"},
    {"〉", "〈"},
    {"」", "「"},
    {"』", "『"},
    {"〗", "〖"},
    {"］", "［"},
    {"｝", "｛"},
};

Jeditor::Jeditor(QWidget *parent)
    : PrimaryEditor(parent)
    , restrict_rule_(new TextRestrictRule)
    , tokenizer_(Tokenizer::build().result()) {
    auto pal = palette();
    pal.setColor(QPalette::Window, QColor("#ffffff"));
    pal.setColor(QPalette::Highlight, QColor("#e3eff6"));
    setPalette(pal);

    set_caret_style(CaretBlinkStyle::Stack);

    set_soft_center_enabled(true);
    set_elastic_resize_enabled(false);
    update_text_view_geometry();

    set_insert_filter_enabled(true);
}

bool Jeditor::soft_center_enabled() const {
    return soft_center_;
}

void Jeditor::set_soft_center_enabled(bool enabled) {
    if (soft_center_ == enabled) { return; }
    soft_center_ = enabled;
    update_text_view_geometry();
}

bool Jeditor::elastic_resize_enabled() const {
    return elastic_resize_;
}

void Jeditor::set_elastic_resize_enabled(bool enabled) {
    if (elastic_resize_ == enabled) { return; }
    elastic_resize_ = enabled;
    if (!elastic_resize_) { update_text_view_geometry(); }
}

int Jeditor::horizontal_margin_hint() const {
    int margin_hint = 8;
    if (soft_center_enabled()) {
        const auto min_margin     = 64;
        const auto max_text_width = 1000;
        const auto mean_width     = qMax(0, width() - min_margin * 2);
        const auto text_width     = qMin<int>(mean_width * 0.8, max_text_width);
        margin_hint               = width() - text_width;
    }
    if (!soft_center_enabled() || !elastic_resize_enabled()) { return margin_hint; }
    const int margin = horizontal_margin();
    if (margin_hint == margin) { return margin_hint; }
    const auto bb         = contentsRect();
    const int  last_width = context()->viewport_width + margin;
    const int  dw         = bb.width() - last_width;
    if (dw >= 0) {
        return qMin(margin_hint, margin + dw);
    } else {
        return qMax(8, margin + dw);
    }
}

Tokenizer *Jeditor::tokenizer() const {
    return tokenizer_;
}

AbstractTextRestrictRule *Jeditor::restrict_rule() const {
    return restrict_rule_;
}

bool Jeditor::insert_action_filter(const QString &text, bool batch_mode) {
    auto ctx = context();

    if (batch_mode) {
        const auto &cursor   = engine()->cursor;
        const int   text_len = engine()->current_block()->text_len();
        if (text == "\n") {
            if (cursor.pos == 0) { return true; }
            if (text_len > 0 && cursor.pos == text_len) { return false; }
        }
    } else {
        if (QUOTE_PAIRS.count(text)) {
            auto matched = QUOTE_PAIRS[text];
            execute_insert_action(text + matched, false);
            ctx->move(-1, false);
            return true;
        }

        if (auto index = QUOTE_PAIRS.values().indexOf(text); index != -1) {
            const int pos = ctx->edit_cursor_pos;
            if (pos == ctx->edit_text.length()) { return false; }
            if (ctx->edit_text.at(pos) == text) { return true; }
        }
    }

    return false;
}

void Jeditor::internal_action_on_leave_block(int block_index) {
    auto e = engine();

    //! TODO: trigger leave block on set-caret

    const auto block = e->active_blocks[block_index];

    if (false) {
        //! TODO: apply restrict rule on block text
        //! TODO: add replace method
    }

    if (block->text_len() == 0) {
        //! TODO: add remove block method
        e->active_blocks.remove(block_index);
        if (e->active_block_index != -1 && e->active_block_index > block_index) {
            --e->active_block_index;
        }
        context()->cached_render_data_ready = false;
    }
}

void Jeditor::execute_text_input_command(TextInputCommand cmd, const QString &text) {
    switch (cmd) {
        case TextInputCommand::Copy: {
            if (!context()->has_sel()) { return; }
        } break;
        case TextInputCommand::InsertTab: {
            const auto e = engine();
            if (!e->is_cursor_available()) { break; }
            const auto &cursor = e->cursor;
            const int   pos    = cursor.pos;
            if (pos == 0 || pos == e->current_block()->text_len()) { break; }
            if (const bool succeed = try_exit_quotes()) { return; }
        } break;
        default: {
        } break;
    }
    PrimaryEditor::execute_text_input_command(cmd, text);
}

void Jeditor::draw(QPainter *p) {
    if (enabled()) {
        PrimaryEditor::draw(p);
        return;
    }

    //! draw a message when the editor is disabled
    auto font = this->font();
    font.setPointSize(10);
    p->setFont(font);
    p->setRenderHint(QPainter::TextAntialiasing);
    p->setPen(QColor(150, 150, 150));

    //! TODO: add message for loading progress
    p->drawText(rect(), Qt::AlignCenter, "双击编辑新章节");
}

bool Jeditor::try_exit_quotes() {
    auto e = engine();
    Q_ASSERT(!e->preedit);
    Q_ASSERT(e->is_cursor_available());

    const auto &cursor = e->cursor;
    const auto  block  = e->current_block();
    const auto &text   = block->text();

    QStack<QString> quote_stack;

    QString left_part{};
    for (int i = cursor.pos - 1; i >= 0; --i) {
        bool found = false;
        for (const auto &[l, r] : QUOTE_PAIRS.asKeyValueRange()) {
            const auto c = text.at(i);
            if (c == l) {
                bool consumed = false;
                while (!quote_stack.empty()) {
                    const auto rpart = quote_stack.pop();
                    if (REVERSE_QUOTE_PAIRS.value(rpart) == l) {
                        consumed = true;
                        break;
                    }
                }
                if (!consumed) {
                    found     = true;
                    left_part = c;
                    break;
                }
            } else if (c == r) {
                quote_stack.push(c);
            }
        }
        if (found) { break; }
    }

    if (left_part.isEmpty()) { return false; }

    Q_ASSERT(QUOTE_PAIRS.contains(left_part));
    const auto right_part = QUOTE_PAIRS.value(left_part);

    const int pos = text.indexOf(right_part, cursor.pos);
    if (pos == -1) { return false; }

    move(pos - cursor.pos + right_part.length(), false);

    return true;
}

bool Jeditor::event(QEvent *event) {
    if (event->type() == QEvent::MouseButtonDblClick) { return QWidget::event(event); }
    return PrimaryEditor::event(event);
}

void Jeditor::keyPressEvent(QKeyEvent *event) {
    auto e = engine();
    if (!e->is_cursor_available() || e->preedit) { return; }

    auto &config = AppConfig::get_instance();
    auto &man    = config.primary_text_input_command_manager();

    auto action = TextInputCommand::Reject;
    if (readonly()) {
        if (event->matches(QKeySequence::MoveToPreviousLine)) {
            action = TextInputCommand::ScrollUp;
        } else if (event->matches(QKeySequence::MoveToNextLine)) {
            action = TextInputCommand::ScrollDown;
        }
    }
    if (action == TextInputCommand::Reject) {
        man.push(e);
        action = man.match(event);
        man.pop();
    }

    QString inserted{};
    if (action == TextInputCommand::InsertPrintable) {
        inserted = TextInputCommandManager::translate_printable_char(event);
    }

    execute_text_input_command(action, inserted);
}

void Jeditor::mouseDoubleClickEvent(QMouseEvent *event) {
    if (enabled()) { return PrimaryEditor::mouseDoubleClickEvent(event); }
    if (event->button() == Qt::LeftButton) {
        set_enabled(true);
        set_readonly(false);
        handle_on_activate();
    }
}

} // namespace jwrite::ui
