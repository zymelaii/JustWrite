#include "Editor.h"
#include "../TextViewEngine.h"
#include "../TextInputCommand.h"
#include "../ProfileUtils.h"
#include <QResizeEvent>
#include <QPaintEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QInputMethodEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QGuiApplication>
#include <QClipboard>
#include <QTimer>
#include <QMap>
#include <magic_enum.hpp>

namespace jwrite::Ui {

Editor::Editor(QWidget *parent)
    : QWidget(parent) {
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAutoFillBackground(true);
    setAcceptDrops(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setContentsMargins({});

    min_text_line_chars_       = 12;
    soft_center_mode_          = false;
    expected_scroll_           = 0.0;
    blink_cursor_should_paint_ = true;
    inserted_filter_enabled_   = true;
    drag_sel_flag_             = false;
    ui_cursor_shape_[0]        = Qt::ArrowCursor;
    ui_cursor_shape_[1]        = Qt::ArrowCursor;

    setSoftCenterMode(true);

    const auto text_area = textArea();
    context_             = new jwrite::VisualTextEditContext(fontMetrics(), text_area.width());
    context_->resize_viewport(context_->viewport_width, text_area.height());

    blink_timer_.setInterval(500);
    blink_timer_.setSingleShot(false);
    auto_scroll_timer_.setInterval(10);
    auto_scroll_timer_.setSingleShot(false);

    input_manager_ = new jwrite::GeneralTextInputCommandManager(context_->engine);
    input_manager_->load_default();

    scrollToStart();

    connect(this, &Editor::textAreaChanged, this, [this](QRect area) {
        context_->resize_viewport(area.width(), area.height());
        update();
    });
    connect(&blink_timer_, &QTimer::timeout, this, [this] {
        blink_cursor_should_paint_ = !blink_cursor_should_paint_;
        update();
    });
    connect(&auto_scroll_timer_, &QTimer::timeout, this, [this] {
        scroll((scroll_ref_y_pos_ - scroll_base_y_pos_) / 10, false);
        update();
    });
}

Editor::~Editor() {
    delete context_;
    delete input_manager_;
}

bool Editor::softCenterMode() const {
    return soft_center_mode_;
}

void Editor::setSoftCenterMode(bool value) {
    soft_center_mode_ = value;
    if (soft_center_mode_) {
        const auto min_margin     = 32;
        const auto max_text_width = 1000;
        const auto mean_width     = qMax(0, width() - min_margin * 2);
        const auto text_width     = qMin<int>(mean_width * 0.8, max_text_width);
        const auto margin         = (width() - text_width) / 2;
        ui_margins_               = QMargins(margin, 4, margin, 4);
    } else {
        ui_margins_ = QMargins(4, 4, 4, 4);
    }
    emit textAreaChanged(textArea());
}

QRect Editor::textArea() const {
    return contentsRect() - ui_margins_;
}

void Editor::reset(QString &text, bool swap) {
    context_->quit_preedit();

    QStringList blocks{};
    for (auto block : context_->engine.active_blocks) { blocks << block->text().toString(); }
    auto text_out = blocks.join("\n");
    context_->edit_text.clear();

    context_->engine.clear_all();
    context_->engine.insert_block(0);
    context_->engine.active_block_index = 0;
    context_->engine.cursor.reset();

    context_->edit_cursor_pos = 0;

    context_->cached_render_data_ready = false;
    context_->cursor_moved             = true;
    context_->vertical_move_state      = false;
    context_->unset_sel();

    insertRawText(text);

    context_->engine.active_block_index = 0;
    context_->edit_cursor_pos           = 0;
    context_->engine.cursor.reset();

    if (swap) { text.swap(text_out); }

    requestUpdate();
}

void Editor::scrollToCursor() {
    const auto &d = context_->cached_render_state;
    const auto &e = context_->engine;
    Q_ASSERT(context_->cached_render_data_ready);
    Q_ASSERT(context_->cursor_moved);

    const auto   viewport     = textArea();
    const auto  &line         = e.current_line();
    const auto  &cursor       = e.cursor;
    const double line_spacing = e.line_height * e.line_spacing_ratio;

    double y_pos = 0.0;
    if (d.active_block_visible) {
        y_pos = d.active_block_y_start;
    } else if (!d.found_visible_block) {
        for (int index = 0; index < e.active_block_index; ++index) {
            y_pos += e.block_spacing + line_spacing * e.active_blocks[index]->lines.size();
        }
    } else if (e.active_block_index < d.visible_block.first) {
        y_pos = d.cached_block_y_pos[d.visible_block.first];
        for (int index = d.visible_block.first; index > e.active_block_index; --index) {
            y_pos -= e.block_spacing + line_spacing * e.active_blocks[index]->lines.size();
        }
    } else if (e.active_block_index > d.visible_block.last) {
        y_pos = d.cached_block_y_pos[d.visible_block.last];
        for (int index = d.visible_block.last; index < e.active_block_index; ++index) {
            y_pos += e.block_spacing + line_spacing * e.active_blocks[index]->lines.size();
        }
    }
    y_pos += cursor.row * line_spacing;

    const double slack       = e.block_spacing * 0.75;
    const double y_pos_start = y_pos - slack;
    const double y_pos_end   = y_pos + line_spacing + slack;

    if (const double top = context_->viewport_y_pos; y_pos_start < top) {
        scrollTo(y_pos_start, true);
    } else if (const double bottom = context_->viewport_y_pos + context_->viewport_height;
               y_pos_end > bottom) {
        scrollTo(y_pos_end - context_->viewport_height, true);
    }

    context_->cursor_moved = false;
}

VisualTextEditContext::TextLoc Editor::currentTextLoc() const {
    VisualTextEditContext::TextLoc loc{.block_index = -1};

    const auto &e = context_->engine;
    if (!e.is_cursor_available()) { return loc; }

    loc.block_index = e.active_block_index;
    loc.row         = e.cursor.row;
    loc.col         = e.cursor.col;
    loc.pos         = e.current_line().text_offset() + e.cursor.col;

    return loc;
}

void Editor::setCursorToTextLoc(const VisualTextEditContext::TextLoc &loc) {
    context_->set_cursor_to_textloc(loc, 0);
}

QPair<double, double> Editor::scrollBound() const {
    const auto  &e            = context_->engine;
    const double line_spacing = e.line_spacing_ratio * e.line_height;

    //! see drawHighlightBlock(QPainter *p)
    const double line_gap  = line_spacing + e.block_spacing;
    const double slack     = e.block_spacing * 0.75;
    const double min_y_pos = -line_gap;
    double       max_y_pos = -line_gap - slack;
    for (auto block : e.active_blocks) {
        max_y_pos += block->lines.size() * line_spacing + e.block_spacing;
    }

    return {min_y_pos, max_y_pos};
}

void Editor::scroll(double delta, bool smooth) {
    const auto [min_y_pos, max_y_pos] = scrollBound();
    expected_scroll_ = qBound(min_y_pos, context_->viewport_y_pos + delta, max_y_pos);
    if (!smooth) { context_->scroll_to(expected_scroll_); }
    update();
}

void Editor::scrollTo(double pos_y, bool smooth) {
    const auto [min_y_pos, max_y_pos] = scrollBound();
    expected_scroll_                  = qBound(min_y_pos, pos_y, max_y_pos);
    if (!smooth) { context_->scroll_to(expected_scroll_); }
    update();
}

void Editor::scrollToStart() {
    const auto [min_y_pos, _] = scrollBound();
    expected_scroll_          = min_y_pos;
    context_->scroll_to(expected_scroll_);
    requestUpdate();
}

void Editor::scrollToEnd() {
    const auto [_, max_y_pos] = scrollBound();
    expected_scroll_          = max_y_pos;
    context_->scroll_to(expected_scroll_);
    requestUpdate();
}

void Editor::insertRawText(const QString &text) {
    auto text_list           = text.split('\n');
    inserted_filter_enabled_ = false;
    for (int i = 0; i < text_list.size(); ++i) {
        auto text = text_list[i].trimmed();
        if (text.isEmpty()) { continue; }
        context_->insert(text);
        if (i + 1 < text_list.size()) { breakIntoNewLine(); }
    }
    inserted_filter_enabled_ = true;
}

bool Editor::insertedPairFilter(const QString &text) {
    static QMap<QString, QString> QUOTE_PAIRS{
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

    if (QUOTE_PAIRS.count(text)) {
        auto matched = QUOTE_PAIRS[text];
        context_->insert(text + matched);
        context_->move(-1, false);
        return true;
    }

    const auto &e = context_->engine;
    if (auto index = QUOTE_PAIRS.values().indexOf(text); index != -1) {
        const int pos = context_->edit_cursor_pos;
        if (pos == context_->edit_text.length()) { return false; }
        if (context_->edit_text.at(pos) == text) { return true; }
    }

    return false;
}

void Editor::select(int start_pos, int end_pos) {
    auto &sel = context_->sel;
    sel.clear();
    sel.from                           = qBound(0, start_pos, context_->edit_text.length());
    sel.to                             = qBound(0, end_pos, context_->edit_text.length());
    context_->cached_render_data_ready = false;
    requestUpdate();
}

void Editor::move(int offset, bool extend_sel) {
    context_->move(offset, extend_sel);
    requestUpdate();
}

void Editor::moveTo(int pos, bool extend_sel) {
    context_->move_to(pos, extend_sel);
    requestUpdate();
}

void Editor::insert(const QString &text) {
    if (context_->engine.preedit) { context_->commit_preedit(); }

    if (inserted_filter_enabled_) {
        if (insertedPairFilter(text)) { return; }
    }

    jwrite_profiler_start(GeneralTextEdit);
    context_->insert(text);
    jwrite_profiler_record(GeneralTextEdit);

    emit textChanged(context_->edit_text);
    requestUpdate();
}

void Editor::del(int times) {
    context_->del(times, false);
    emit textChanged(context_->edit_text);
    requestUpdate();
}

void Editor::copy() {
    auto clipboard = QGuiApplication::clipboard();
    if (context_->has_sel()) {
        const auto &e        = context_->engine;
        const int   pos_from = qMin(context_->sel.from, context_->sel.to);
        const int   pos_to   = qMax(context_->sel.from, context_->sel.to);
        const auto  loc_from = context_->get_textloc_at_pos(pos_from, -1);
        const auto  loc_to   = context_->get_textloc_at_pos(pos_to, 1);
        QStringList copied_text{};
        for (int index = loc_from.block_index; index <= loc_to.block_index; ++index) {
            const auto block = e.active_blocks[index];
            const int  from  = index == loc_from.block_index ? loc_from.pos : 0;
            const int  to    = index == loc_to.block_index ? loc_to.pos : block->text_len();
            copied_text << block->text().mid(from, to - from).toString();
        }
        clipboard->setText(copied_text.join('\n'));
    } else if (const auto &e = context_->engine; e.is_cursor_available()) {
        //! copy the current block if has no sel
        const auto copied_text = e.current_block()->text();
        clipboard->setText(copied_text.toString());
    }
}

void Editor::cut() {
    copy();
    if (context_->has_sel()) {
        context_->remove_sel_region();
    } else {
        //! cut the current block if has no sel, also remove the block
        const auto &e = context_->engine;
        context_->move(-e.cursor.pos, false);
        context_->del(e.current_block()->text_len() + 1, false);
    }
    requestUpdate();
}

void Editor::paste() {
    auto clipboard = QGuiApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    insertRawText(clipboard->text());
    requestUpdate();
}

void Editor::breakIntoNewLine() {
    context_->remove_sel_region();
    context_->engine.break_block_at_cursor_pos();
    context_->cursor_moved = true;
    requestUpdate();
}

void Editor::verticalMove(bool up) {
    context_->vertical_move(up);
    requestUpdate();
}

void Editor::requestUpdate() {
    blink_cursor_should_paint_ = true;
    blink_timer_.stop();
    update();
    blink_timer_.start();
}

void Editor::setCursorShape(Qt::CursorShape shape) {
    ui_cursor_shape_[1] = ui_cursor_shape_[0];
    ui_cursor_shape_[0] = shape;
    setCursor(shape);
}

void Editor::restoreCursorShape() {
    setCursorShape(ui_cursor_shape_[1]);
}

QSize Editor::sizeHint() const {
    return minimumSizeHint();
}

QSize Editor::minimumSizeHint() const {
    const auto margins      = contentsMargins() + ui_margins_;
    const auto line_spacing = context_->engine.line_height * context_->engine.line_spacing_ratio;
    const auto hori_margin  = margins.left() + margins.right();
    const auto vert_margin  = margins.top() + margins.bottom();
    const auto min_width =
        min_text_line_chars_ * context_->engine.standard_char_width + hori_margin;
    const auto min_height = line_spacing * 3 + context_->engine.block_spacing * 2 + vert_margin;
    return QSize(min_width, min_height);
}

void Editor::drawTextArea(QPainter *p) {
    const auto &d = context_->cached_render_state;
    if (!d.found_visible_block) { return; }

    jwrite_profiler_start(TextBodyRenderCost);

    const auto pal   = palette();
    const auto flags = Qt::AlignBaseline | Qt::TextDontClip;

    p->save();
    p->setPen(pal.color(QPalette::Text));

    const auto  &e            = context_->engine;
    const double indent       = e.standard_char_width * 2;
    const double line_spacing = e.line_height * e.line_spacing_ratio;
    const auto   viewport     = textArea();

    QRectF bb(viewport.left(), viewport.top(), viewport.width(), e.line_height);
    bb.translate(0, d.first_visible_block_y_pos - context_->viewport_y_pos);

    const auto y_pos = d.first_visible_block_y_pos - context_->viewport_y_pos;
    auto       pos   = viewport.topLeft() + QPointF(0, y_pos);

    for (int index = d.visible_block.first; index <= d.visible_block.last; ++index) {
        const auto block = e.active_blocks[index];

        for (const auto &line : block->lines) {
            const double leading_space = line.is_first_line() ? indent : 0;
            const double spacing       = line.char_spacing();
            bb.setLeft(viewport.left() + leading_space);
            for (const auto c : line.text()) {
                p->drawText(bb, flags, c);
                const double advance = e.fm.horizontalAdvance(c);
                bb.adjust(advance + spacing, 0, 0, 0);
            }
            bb.translate(0, line_spacing);
        }
        bb.translate(0, e.block_spacing);
    }

    p->restore();

    jwrite_profiler_record(TextBodyRenderCost);
}

void Editor::drawSelection(QPainter *p) {
    const auto &e = context_->engine;
    const auto &d = context_->cached_render_state;
    if (!context_->has_sel()) { return; }
    if (d.visible_sel.first == -1) { return; }

    jwrite_profiler_start(SelectionAreaRenderCost);

    const auto pal = palette();

    const auto   viewport     = textArea();
    const double line_spacing = e.line_height * e.line_spacing_ratio;

    double y_pos = d.cached_block_y_pos[d.visible_sel.first];

    for (int index = d.visible_sel.first; index <= d.visible_sel.last; ++index) {
        const auto   block         = e.active_blocks[index];
        const bool   is_first      = index == d.sel_loc_from.block_index;
        const bool   is_last       = index == d.sel_loc_to.block_index;
        const int    line_nr_begin = is_first ? d.sel_loc_from.row : 0;
        const int    line_nr_end   = is_last ? d.sel_loc_to.row : block->lines.size() - 1;
        const double stride        = line_nr_begin * line_spacing;

        double y_pos = d.cached_block_y_pos[index] + stride - context_->viewport_y_pos;
        //! add the minus part to make the selection area align center
        y_pos -= e.fm.descent() * 0.5;

        for (int line_nr = line_nr_begin; line_nr <= line_nr_end; ++line_nr) {
            const auto line   = block->lines[line_nr];
            const auto offset = line.is_first_line() ? e.standard_char_width * 2 : 0;

            double x_pos = offset;
            if (is_first && line_nr == d.sel_loc_from.row) {
                x_pos += line.char_spacing() * d.sel_loc_from.col;
                for (const auto &c : line.text().left(d.sel_loc_from.col)) {
                    x_pos += e.fm.horizontalAdvance(c);
                }
            }

            double sel_width = line.cached_text_width + (line.text_len() - 1) * line.char_spacing()
                             - (x_pos - offset);
            if (is_last && line_nr == d.sel_loc_to.row) {
                sel_width -= line.char_spacing() * (line.text_len() - d.sel_loc_to.col);
                for (const auto &c : line.text().mid(d.sel_loc_to.col)) {
                    sel_width -= e.fm.horizontalAdvance(c);
                }
            }

            QRectF bb(x_pos, y_pos, sel_width, e.line_height);
            bb.translate(viewport.topLeft());
            p->fillRect(bb, pal.highlightedText());

            y_pos += line_spacing;
        }
    }

    jwrite_profiler_record(SelectionAreaRenderCost);
}

void Editor::drawHighlightBlock(QPainter *p) {
    const auto &d = context_->cached_render_state;
    const auto &e = context_->engine;
    if (!e.is_cursor_available() || context_->has_sel()) { return; }
    if (!d.active_block_visible) { return; }

    const auto pal = palette();

    p->save();
    p->setPen(Qt::transparent);
    p->setBrush(pal.highlight());

    const auto viewport = textArea();

    const double line_spacing = e.line_height * e.line_spacing_ratio;
    const double line_slack   = qMax(0.0, line_spacing - e.line_height);
    const double start_y_pos  = d.active_block_y_start - context_->viewport_y_pos - line_slack;
    const double end_y_pos    = d.active_block_y_end - context_->viewport_y_pos - e.block_spacing;

    const double height  = end_y_pos - start_y_pos;
    const double w_slack = 8.0;
    const double h_slack = context_->engine.block_spacing * 0.75;
    const int    radius  = 4;

    QRectF bb(0, start_y_pos, context_->viewport_width, height);
    bb.translate(viewport.topLeft());
    bb.adjust(-w_slack, -h_slack, w_slack, h_slack);

    p->drawRoundedRect(bb, radius, radius);

    p->restore();
}

void Editor::drawCursor(QPainter *p) {
    const auto &d = context_->cached_render_state;
    const auto &e = context_->engine;
    if (!(e.is_cursor_available() && blink_cursor_should_paint_)) { return; }
    if (!d.active_block_visible) { return; }

    jwrite_profiler_start(CursorRenderCost);

    const auto   viewport     = textArea();
    const auto  &line         = e.current_line();
    const auto  &cursor       = e.cursor;
    const double line_spacing = e.line_height * e.line_spacing_ratio;
    const double y_pos        = d.active_block_y_start + cursor.row * line_spacing;

    //! NOTE: you may question about why it doesn't call `fm.horizontalAdvance(text)`
    //! directly, and the reason is that the text_width calcualated by that has a few
    //! difference with the render result of the text, and the cursor will seems not in the
    //! correct place, and this problem was extremely serious in pure latin texts
    const double leading_space = line.is_first_line() ? e.standard_char_width * 2 : 0;
    double       cursor_x_pos  = leading_space + line.char_spacing() * cursor.col;
    for (const auto &c : line.text().left(cursor.col)) {
        cursor_x_pos += e.fm.horizontalAdvance(c);
    }
    const double cursor_y_pos = y_pos - context_->viewport_y_pos;
    const auto   cursor_pos   = QPoint(cursor_x_pos, cursor_y_pos) + viewport.topLeft();

    p->save();

    //! NOTE: set pen width less than 1 to ensure a single pixel cursor
    p->setPen(QPen(palette().text(), 0.8));

    p->drawLine(cursor_pos, cursor_pos + QPoint(0, e.fm.height()));

    p->restore();

    jwrite_profiler_record(CursorRenderCost);
}

void Editor::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (soft_center_mode_) { setSoftCenterMode(true); }
    emit textAreaChanged(textArea());
}

void Editor::paintEvent(QPaintEvent *e) {
    jwrite_profiler_start(FrameRenderCost);

    QPainter p(this);
    auto     pal = palette();

    //! smooth scroll
    if (qAbs(context_->viewport_y_pos - expected_scroll_) > 1e-3) {
        const double new_scroll_pos = context_->viewport_y_pos * 0.49 + expected_scroll_ * 0.51;
        const double scroll_delta   = new_scroll_pos - context_->viewport_y_pos;
        if (qAbs(scroll_delta) < 10) {
            context_->scroll_to(expected_scroll_);
        } else {
            context_->scroll_to(new_scroll_pos);
            QTimer::singleShot(10, [this]() {
                update();
            });
        }
    }

    //! prepare render data
    jwrite_profiler_start(PrepareRenderData);
    context_->prepare_render_data();
    jwrite_profiler_record(PrepareRenderData);

    //! draw selection
    drawSelection(&p);

    //! draw highlight text block
    drawHighlightBlock(&p);

    //! draw text area
    drawTextArea(&p);

    //! draw cursor
    drawCursor(&p);
    if (context_->cursor_moved) { scrollToCursor(); }

    jwrite_profiler_record(IME2UpdateDelay);
    jwrite_profiler_record(FrameRenderCost);
}

void Editor::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);
    if (auto &e = context_->engine; e.is_empty()) {
        e.insert_block(0);
        //! TODO: move it into a safe method
        e.active_block_index = 0;
        emit activated();
    }
    requestUpdate();
}

void Editor::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    context_->unset_sel();
    blink_timer_.stop();
    context_->engine.active_block_index = -1;
    emit focusLost();
}

void Editor::keyPressEvent(QKeyEvent *e) {
    if (!context_->engine.is_cursor_available()) { return; }

    //! ATTENTION: normally this branch is unreachable, but when IME events are too frequent and
    //! the system latency is too high, an IME preedit interrupt may occur and the key is
    //! forwarded to the keyPress event. in this case, we should reject the event or submit the
    //! raw preedit text. the first solution is taken here.
    //! FIXME: the solution is not fully tested to be safe and correct
    if (context_->engine.preedit) { return; }

    const auto action = input_manager_->match(e);
    qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

    auto        &engine       = context_->engine;
    auto        &cursor       = engine.cursor;
    const double line_spacing = engine.line_height * engine.line_spacing_ratio;

    jwrite_profiler_start(GeneralTextEdit);

    switch (action) {
        case TextInputCommand::Reject: {
        } break;
        case TextInputCommand::InsertPrintable: {
            insert(input_manager_->translate_printable_char(e));
        } break;
        case TextInputCommand::InsertTab: {
        } break;
        case TextInputCommand::InsertNewLine: {
            breakIntoNewLine();
        } break;
        case TextInputCommand::Cancel: {
            context_->unset_sel();
            update();
        } break;
        case TextInputCommand::Undo: {
        } break;
        case TextInputCommand::Redo: {
        } break;
        case TextInputCommand::Copy: {
            copy();
        } break;
        case TextInputCommand::Cut: {
            cut();
        } break;
        case TextInputCommand::Paste: {
            paste();
        } break;
        case TextInputCommand::ScrollUp: {
            scroll(-line_spacing, true);
        } break;
        case TextInputCommand::ScrollDown: {
            scroll(line_spacing, true);
        } break;
        case TextInputCommand::MoveToPrevChar: {
            move(-1, false);
        } break;
        case TextInputCommand::MoveToNextChar: {
            move(1, false);
        } break;
        case TextInputCommand::MoveToPrevWord: {
        } break;
        case TextInputCommand::MoveToNextWord: {
        } break;
        case TextInputCommand::MoveToPrevLine: {
            verticalMove(true);
        } break;
        case TextInputCommand::MoveToNextLine: {
            verticalMove(false);
        } break;
        case TextInputCommand::MoveToStartOfLine: {
            const auto block = engine.current_block();
            const auto line  = block->current_line();
            moveTo(block->text_pos + line.text_offset(), false);
        } break;
        case TextInputCommand::MoveToEndOfLine: {
            const auto block = engine.current_block();
            const auto line  = block->current_line();
            const auto pos   = block->text_pos + line.text_offset() + line.text().length();
            moveTo(pos, false);
        } break;
        case TextInputCommand::MoveToStartOfBlock: {
            moveTo(engine.current_block()->text_pos, false);
        } break;
        case TextInputCommand::MoveToEndOfBlock: {
            const auto block = engine.current_block();
            moveTo(block->text_pos + block->text_len(), false);
        } break;
        case TextInputCommand::MoveToStartOfDocument: {
            moveTo(0, false);
            scrollToStart();
        } break;
        case TextInputCommand::MoveToEndOfDocument: {
            moveTo(engine.text_ref->length(), false);
            scrollToEnd();
        } break;
        case TextInputCommand::MoveToPrevPage: {
            scroll(-textArea().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToNextPage: {
            scroll(textArea().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToPrevBlock: {
            if (engine.active_block_index > 0) {
                const auto block      = engine.current_block();
                const auto prev_block = engine.active_blocks[engine.active_block_index - 1];
                move(prev_block->text_pos - block->text_pos - cursor.pos - 1, false);
            }
        } break;
        case TextInputCommand::MoveToNextBlock: {
            if (engine.active_block_index + 1 < engine.active_blocks.size()) {
                const auto block      = engine.current_block();
                const auto next_block = engine.active_blocks[engine.active_block_index + 1];
                move(next_block->text_pos - block->text_pos - cursor.pos + 1, false);
            }
        } break;
        case TextInputCommand::DeletePrevChar: {
            del(-1);
        } break;
        case TextInputCommand::DeleteNextChar: {
            del(1);
        } break;
        case TextInputCommand::DeletePrevWord: {
        } break;
        case TextInputCommand::DeleteNextWord: {
        } break;
        case TextInputCommand::DeleteToStartOfLine: {
            const auto &block = engine.current_block();
            int         times = cursor.col;
            if (times == 0) {
                if (cursor.row > 0) {
                    times = block->len_of_line(cursor.row - 1);
                } else {
                    times = 1;
                }
            }
            del(-times);
        } break;
        case TextInputCommand::DeleteToEndOfLine: {
            const auto &block = engine.current_block();
            int         times = block->len_of_line(cursor.row) - cursor.col;
            if (times == 0) {
                if (cursor.row + 1 == block->lines.size()) {
                    times = 1;
                } else {
                    times = block->len_of_line(cursor.row + 1);
                }
            }
            del(times);
        } break;
        case TextInputCommand::DeleteToStartOfBlock: {
        } break;
        case TextInputCommand::DeleteToEndOfBlock: {
        } break;
        case TextInputCommand::DeleteToStartOfDocument: {
        } break;
        case TextInputCommand::DeleteToEndOfDocument: {
        } break;
        case TextInputCommand::SelectPrevChar: {
            move(-1, true);
        } break;
        case TextInputCommand::SelectNextChar: {
            move(1, true);
        } break;
        case TextInputCommand::SelectPrevWord: {
        } break;
        case TextInputCommand::SelectNextWord: {
        } break;
        case TextInputCommand::SelectToPrevLine: {
            auto     &c        = *context_;
            const int sel_from = c.has_sel() ? c.sel.from : c.edit_cursor_pos;
            if (c.has_sel()) { c.unset_sel(); }
            verticalMove(true);
            c.sel.from = sel_from;
            c.sel.to   = c.edit_cursor_pos;
        } break;
        case TextInputCommand::SelectToNextLine: {
            auto     &c        = *context_;
            const int sel_from = c.has_sel() ? c.sel.from : c.edit_cursor_pos;
            if (c.has_sel()) { c.unset_sel(); }
            verticalMove(false);
            c.sel.from = sel_from;
            c.sel.to   = c.edit_cursor_pos;
        } break;
        case TextInputCommand::SelectToStartOfLine: {
            move(-cursor.col, true);
        } break;
        case TextInputCommand::SelectToEndOfLine: {
            move(engine.current_line().text().length() - cursor.col, true);
        } break;
        case TextInputCommand::SelectToStartOfBlock: {
            moveTo(engine.current_block()->text_pos, true);
        } break;
        case TextInputCommand::SelectToEndOfBlock: {
            const auto block = engine.current_block();
            moveTo(block->text_pos + block->text_len(), true);
        } break;
        case TextInputCommand::SelectBlock: {
            const auto block = engine.current_block();
            select(block->text_pos, block->text_pos + block->text_len());
        } break;
        case TextInputCommand::SelectPrevPage: {
        } break;
        case TextInputCommand::SelectNextPage: {
        } break;
        case TextInputCommand::SelectToStartOfDoc: {
            moveTo(0, true);
        } break;
        case TextInputCommand::SelectToEndOfDoc: {
            moveTo(engine.text_ref->length(), true);
        } break;
        case TextInputCommand::SelectAll: {
            select(0, context_->engine.text_ref->length());
        } break;
        case TextInputCommand::InsertBeforeBlock: {
            engine.insert_block(engine.active_block_index);
            --engine.active_block_index;
            cursor.reset();
            context_->edit_cursor_pos = engine.current_block()->text_pos;
            requestUpdate();
        } break;
        case TextInputCommand::InsertAfterBlock: {
            engine.insert_block(engine.active_block_index + 1);
            auto block = engine.current_block();
            move(block->text_len() - cursor.pos + 1, false);
        } break;
    }

    jwrite_profiler_record(GeneralTextEdit);

    e->accept();
}

void Editor::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    bool cancel_auto_scroll = false;

    if (e->button() == Qt::MiddleButton) {
        auto_scroll_mode_ = !auto_scroll_mode_;
    } else if (auto_scroll_mode_) {
        cancel_auto_scroll = true;
        auto_scroll_mode_  = false;
    }

    if (auto_scroll_mode_) {
        setCursorShape(Qt::SizeVerCursor);
        auto_scroll_mode_  = true;
        scroll_base_y_pos_ = e->pos().y();
        scroll_ref_y_pos_  = scroll_base_y_pos_;
        auto_scroll_timer_.start();
        return;
    } else {
        auto_scroll_mode_ = false;
        auto_scroll_timer_.stop();
        restoreCursorShape();
    }

    //! NOTE: do not perform the locating action if the current click is to cancel the auto scroll
    //! mode, so that user could have more choices when they scroll to a different view pos
    if (cancel_auto_scroll) { return; }

    if (e->button() != Qt::LeftButton) {
        drag_sel_flag_ = false;
        return;
    }

    context_->unset_sel();

    auto &engine = context_->engine;

    if (engine.is_empty() || engine.is_dirty()) { return; }
    if (engine.preedit) { return; }

    const auto loc = context_->get_textloc_at_vpos(e->pos() - textArea().topLeft());
    Q_ASSERT(loc.block_index != -1);

    const bool success = context_->set_cursor_to_textloc(loc, 0);
    Q_ASSERT(success);

    requestUpdate();

    drag_sel_flag_ = true;
}

void Editor::mouseReleaseEvent(QMouseEvent *e) {
    QWidget::mouseReleaseEvent(e);

    if (e->buttons() & Qt::LeftButton) { drag_sel_flag_ = false; }
}

void Editor::mouseDoubleClickEvent(QMouseEvent *e) {
    QWidget::mouseDoubleClickEvent(e);

    if (context_->engine.preedit) { return; }

    context_->unset_sel();

    if (e->button() == Qt::LeftButton) {
        const auto loc = context_->get_textloc_at_vpos(e->pos() - textArea().topLeft());
        if (loc.block_index != -1) {
            const auto block = context_->engine.active_blocks[loc.block_index];
            select(block->text_pos, block->text_pos + block->text_len());
        }
    }
}

void Editor::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);

    if (auto_scroll_mode_) {
        scroll_ref_y_pos_ = e->pos().y();
        return;
    }

    if ((e->buttons() & Qt::LeftButton) && drag_sel_flag_) {
        do {
            auto &engine = context_->engine;
            if (!engine.is_cursor_available()) { break; }
            if (engine.preedit) { break; }
            const auto bb   = textArea();
            const auto vpos = e->globalPosition().toPoint() - mapToGlobal(bb.topLeft());
            const auto loc  = context_->get_textloc_at_vpos(vpos);
            Q_ASSERT(loc.block_index != -1);
            const auto block = engine.active_blocks[loc.block_index];
            moveTo(block->text_pos + loc.pos, true);
            const double line_spacing = engine.line_height * engine.line_spacing_ratio;
            if (vpos.y() < 0) {
                scroll(-line_spacing * 0.5, true);
            } else if (vpos.y() > context_->viewport_height) {
                scroll(line_spacing * 0.5, true);
            }
        } while (0);
    }

    const auto &area = textArea();
    if (area.contains(e->pos())) {
        setCursorShape(Qt::IBeamCursor);
    } else {
        setCursorShape(Qt::ArrowCursor);
    }
}

void Editor::wheelEvent(QWheelEvent *e) {
    const auto &engine = context_->engine;
    if (engine.is_empty()) { return; }
    const double ratio        = 1.0 / 180 * 3;
    const double line_spacing = engine.line_height * engine.line_spacing_ratio;
    const double delta        = -e->angleDelta().y() * line_spacing * ratio;
    scroll((e->modifiers() & Qt::ControlModifier) ? delta * 8 : delta, true);
}

void Editor::inputMethodEvent(QInputMethodEvent *e) {
    auto &engine = context_->engine;
    if (!engine.is_cursor_available()) { return; }
    if (const auto preedit_text = e->preeditString(); !preedit_text.isEmpty()) {
        if (!engine.preedit) { context_->begin_preedit(); }
        context_->update_preedit(e->preeditString());
        jwrite_profiler_start(IME2UpdateDelay);
    } else {
        insert(e->commitString());
    }
    requestUpdate();
}

void Editor::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        e->ignore();
    }
}

void Editor::dropEvent(QDropEvent *e) {
    QWidget::dropEvent(e);
    const auto urls = e->mimeData()->urls();
    //! TODO: filter plain text files and handle open action
}

} // namespace jwrite::Ui
