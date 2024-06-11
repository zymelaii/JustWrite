#include <jwrite/ui/SettingsDialog.h>
#include <jwrite/ui/ShortcutEditor.h>
#include <jwrite/AppConfig.h>
#include <jwrite/Version.h>
#include <qt-material/qtmaterialscrollbar.h>
#include <qt-material/qtmaterialtoggle.h>
#include <widget-kit/LayoutHelper.h>
#include <QSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QFrame>
#include <QLabel>
#include <any>
#include <magic_enum.hpp>

using namespace widgetkit;

namespace jwrite::ui {

void new_group(QVBoxLayout *layout, const QString &name) {
    auto w = new QLabel(name);
    w->setFont(AppConfig::get_instance().font(AppConfig::FontStyle::Bold, 10));
    w->setContentsMargins(8, 0, 0, 0);
    auto pal = w->palette();
    pal.setColor(QPalette::WindowText, QColor(150, 150, 150));
    w->setPalette(pal);
    layout->addSpacing(20);
    layout->addWidget(w);
    layout->addSpacing(8);
}

FlatButton *new_settings_panel(QVBoxLayout *layout, const QString &name) {
    auto w = new FlatButton;
    w->setRadius(4);
    w->setText(name);
    w->setTextAlignment(Qt::AlignLeft);
    w->setBorderVisible(false);
    w->setFont(AppConfig::get_instance().font(AppConfig::FontStyle::Light, 13));
    w->setContentsMargins(8, 0, 0, 0);
    auto pal = w->palette();
    pal.setColor(QPalette::Base, QColor(245, 245, 245));
    pal.setColor(QPalette::Button, QColor(229, 229, 229));
    pal.setColor(QPalette::WindowText, QColor(15, 15, 15));
    w->setPalette(pal);
    layout->addWidget(w);
    return w;
}

struct SettingsPanelBuilder;

struct SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    SettingsItemBuilder(SettingsPanelBuilder &parent)
        : parent_{parent} {}

public:
    SettingsPanelBuilder &complete();

protected:
    virtual QWidget *build() = 0;

private:
    SettingsPanelBuilder &parent_;
};

class ComboBuilder : public SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    ComboBuilder(SettingsPanelBuilder &parent)
        : SettingsItemBuilder(parent) {}

public:
    ComboBuilder &with_item(const QString &key, const QString &value) {
        items_.append({key, value});
        return *this;
    }

    ComboBuilder &with_source(AppConfig::ValOption option) {
        option_ = option;
        return *this;
    }

protected:
    QWidget *build() override {
        auto      &config = AppConfig::get_instance();
        const auto it     = std::find_if(items_.begin(), items_.end(), [&](const auto &item) {
            return item.second == config.value(option_);
        });
        Q_ASSERT(it != items_.end());
        auto combo = new QComboBox;
        for (auto &item : items_) { combo->addItem(std::move(item.first), std::move(item.second)); }
        for (int i = 0; i < combo->count(); ++i) {
            combo->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
        }
        combo->setCurrentText(it->first);
        combo->setStyleSheet(R"(
            QComboBox {
                color: #666666;
                font-size: 12pt;
                padding: 4px 10px 4px 10px;
                border: 1px solid rgba(228,228,228,1);
                border-radius: 5px 5px 0px 0px;
            }
            QComboBox::drop-down {
                subcontrol-origin: padding;
                subcontrol-position: top right;
                width: 0px;
                border: none;
            }
            QComboBox QAbstractItemView {
                background: #ffffff;
                border: 1px solid #e4e4e4;
                border-radius: 0px 0px 5px 5px;
                outline: 0px;
            }
            QComboBox QAbstractItemView::item {
                height: 32px;
                color: #666666;
                background-color: #ffffff;
            }
            QComboBox QAbstractItemView::item:hover {
                background-color: #409ce1;
                color: #ffffff;
            }
            QComboBox QAbstractItemView::item:selected {
                background-color: #409ce1;
                color: #ffffff;
            }
        )");
        combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QObject::connect(
            combo,
            &QComboBox::currentIndexChanged,
            &config,
            [&, sender = combo, opt = option_](int index) {
                config.set_value(opt, sender->itemData(index).toString());
            });
        return combo;
    }

private:
    QList<QPair<QString, QString>> items_;
    AppConfig::ValOption           option_;
};

class SpinBuilder : public SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    SpinBuilder(SettingsPanelBuilder &parent)
        : SettingsItemBuilder(parent) {}

public:
    SpinBuilder &with_bounds(int minval, int maxval) {
        Q_ASSERT(minval <= maxval);
        minval_ = minval;
        maxval_ = maxval;
        return *this;
    }

    SpinBuilder &with_step(int step) {
        step_ = step;
        return *this;
    }

    SpinBuilder &with_prefix(const QString &prefix) {
        prefix_ = prefix;
        return *this;
    }

    SpinBuilder &with_suffix(const QString &suffix) {
        suffix_ = suffix;
        return *this;
    }

    SpinBuilder &with_source(AppConfig::ValOption option) {
        option_ = option;
        return *this;
    }

protected:
    QWidget *build() override {
        auto     &config = AppConfig::get_instance();
        bool      ok     = false;
        const int value  = config.value(option_).toInt(&ok);
        Q_ASSERT(ok);
        auto spin = new QSpinBox;
        spin->setMinimum(minval_);
        spin->setMaximum(maxval_);
        spin->setSingleStep(step_);
        spin->setValue(qBound(minval_, value, maxval_));
        spin->setPrefix(prefix_);
        spin->setSuffix(suffix_);
        spin->setStyleSheet(R"(
            QSpinBox {
                font-size: 12pt;
                padding: 4px 10px 4px 10px;
                color: #000000;
                background-color: #ffffff;
                border: 1px solid rgba(228,228,228,1);
                border-radius: 5px 5px 0px 0px;
            }
        )");
        spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spin->setAlignment(Qt::AlignCenter);
        QObject::connect(spin, &QSpinBox::valueChanged, &config, [&, opt = option_](int value) {
            config.set_value(opt, QString::number(value));
        });
        return spin;
    }

private:
    AppConfig::ValOption option_;
    int                  minval_;
    int                  maxval_;
    int                  step_;
    QString              prefix_;
    QString              suffix_;
};

class DoubleSpinBuilder : public SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    DoubleSpinBuilder(SettingsPanelBuilder &parent)
        : SettingsItemBuilder(parent) {}

public:
    DoubleSpinBuilder &with_bounds(double minval, double maxval) {
        Q_ASSERT(minval <= maxval);
        minval_ = minval;
        maxval_ = maxval;
        return *this;
    }

    DoubleSpinBuilder &with_step(double step) {
        step_ = step;
        return *this;
    }

    DoubleSpinBuilder &with_prefix(const QString &prefix) {
        prefix_ = prefix;
        return *this;
    }

    DoubleSpinBuilder &with_suffix(const QString &suffix) {
        suffix_ = suffix;
        return *this;
    }

    DoubleSpinBuilder &with_source(AppConfig::ValOption option) {
        option_ = option;
        return *this;
    }

protected:
    QWidget *build() override {
        auto        &config = AppConfig::get_instance();
        bool         ok     = false;
        const double value  = config.value(option_).toDouble(&ok);
        Q_ASSERT(ok);
        auto spin = new QDoubleSpinBox;
        spin->setMinimum(minval_);
        spin->setMaximum(maxval_);
        spin->setSingleStep(step_);
        spin->setValue(qBound(minval_, value, maxval_));
        spin->setPrefix(prefix_);
        spin->setSuffix(suffix_);
        spin->setStyleSheet(R"(
            QDoubleSpinBox {
                font-size: 12pt;
                padding: 4px 10px 4px 10px;
                color: #000000;
                background-color: #ffffff;
                border: 1px solid rgba(228,228,228,1);
                border-radius: 5px 5px 0px 0px;
            }
        )");
        spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spin->setAlignment(Qt::AlignCenter);
        QObject::connect(
            spin, &QDoubleSpinBox::valueChanged, &config, [&, opt = option_](double value) {
                config.set_value(opt, QString::number(value));
            });
        return spin;
    }

private:
    AppConfig::ValOption option_;
    double               minval_;
    double               maxval_;
    double               step_;
    QString              prefix_;
    QString              suffix_;
};

class ToggleBuilder : public SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    ToggleBuilder(SettingsPanelBuilder &parent)
        : SettingsItemBuilder(parent) {}

public:
    ToggleBuilder &with_source(AppConfig::Option option) {
        option_ = option;
        return *this;
    }

protected:
    QWidget *build() override {
        auto &config = AppConfig::get_instance();
        auto  toggle = new QtMaterialToggle;
        toggle->setChecked(config.option_enabled(option_));
        QObject::connect(
            toggle, &QtMaterialToggle::toggled, &config, [&, opt = option_](bool checked) {
                config.set_option(opt, checked);
            });
        return toggle;
    }

private:
    AppConfig::Option option_;
};

class ButtonBuilder : public SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    ButtonBuilder(SettingsPanelBuilder &parent)
        : SettingsItemBuilder(parent) {}

public:
    ButtonBuilder &with_text(const QString &text) {
        text_ = text;
        return *this;
    }

    ButtonBuilder &with_signal(SettingsDialog *sender, void (SettingsDialog::*signal)()) {
        sender_ = sender;
        signal_ = signal;
        return *this;
    }

protected:
    QWidget *build() override {
        auto button = new FlatButton;
        button->setRadius(4);
        button->setText(text_);
        button->setBorderVisible(true);
        button->setTextAlignment(Qt::AlignCenter);
        button->setContentsMargins(12, 0, 12, 0);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        auto font = button->font();
        font.setPointSize(12);
        button->setFont(font);
        auto pal = button->palette();
        pal.setColor(QPalette::Base, "#7c95a2");
        pal.setColor(QPalette::Button, "#6a8695");
        pal.setColor(QPalette::Window, "#6d838e");
        pal.setColor(QPalette::WindowText, "#ffffff");
        button->setPalette(pal);
        if (sender_ && signal_) {
            QObject::connect(button, &FlatButton::pressed, sender_, signal_);
        }
        return button;
    }

private:
    QString         text_;
    SettingsDialog *sender_;
    void (SettingsDialog::*signal_)();
};

class ShortcutBuilder : public SettingsItemBuilder {
    friend SettingsPanelBuilder;

protected:
    ShortcutBuilder(SettingsPanelBuilder &parent)
        : SettingsItemBuilder(parent) {}

public:
    ShortcutBuilder &with_action(GlobalCommand command) {
        command_ = command;
        return *this;
    }

protected:
    QWidget *build() override {
        auto &config   = AppConfig::get_instance();
        auto &man      = GlobalCommandManager::get_instance();
        auto  shortcut = new ShortcutEditor;
        shortcut->set_shortcut(man.get(command_));
        shortcut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QObject::connect(
            shortcut,
            &ShortcutEditor::on_shortcut_change,
            &config,
            [&, cmd = command_](const QKeySequence &shortcut) {
                man.update_command_shortcut(cmd, shortcut);
            });
        return shortcut;
    }

private:
    GlobalCommand command_;
};

struct SettingsPanelBuilder {
    friend SettingsItemBuilder;

protected:
    struct SettingsItem {
        QString  name;
        QString  desc;
        QWidget *widget;
    };

    struct PanelItem {
        bool     is_group_item;
        std::any item;
    };

public:
    SettingsPanelBuilder(QVBoxLayout *layout)
        : layout_{layout} {}

protected:
    void commit(QWidget *widget) {
        Q_ASSERT(!items_.empty());
        auto &item = items_.last();
        Q_ASSERT(!item.is_group_item);
        auto settings_item = std::any_cast<SettingsItem>(item.item);
        Q_ASSERT(!settings_item.widget);
        Q_ASSERT(widget);
        settings_item.widget = widget;
        item.item            = settings_item;
    }

    void prepare_next_item(const QString &name, const QString &desc) {
        items_.append({
            .is_group_item = false,
            .item          = SettingsItem{name, desc, nullptr},
        });
    }

    template <typename Builder>
    Builder new_item(const QString &name, const QString &desc) {
        prepare_next_item(name, desc);
        return Builder(*this);
    }

public:
    SettingsPanelBuilder &with_group(const QString &name) {
        items_.append({
            .is_group_item = true,
            .item          = name,
        });
        return *this;
    }

    auto with_combo(const QString &name, const QString &desc) {
        return new_item<ComboBuilder>(name, desc);
    }

    auto with_spin(const QString &name, const QString &desc) {
        return new_item<SpinBuilder>(name, desc);
    }

    auto with_double_spin(const QString &name, const QString &desc) {
        return new_item<DoubleSpinBuilder>(name, desc);
    }

    auto with_toggle(const QString &name, const QString &desc) {
        return new_item<ToggleBuilder>(name, desc);
    }

    auto with_button(const QString &name, const QString &desc) {
        return new_item<ButtonBuilder>(name, desc);
    }

    auto with_shortcut(const QString &name, const QString &desc) {
        return new_item<ShortcutBuilder>(name, desc);
    }

protected:
    QWidget *build_settings_desc(const QString &name, const QString &desc) {
        auto container = new QWidget;
        auto layout    = new QVBoxLayout(container);
        layout->setContentsMargins({});
        layout->setSpacing(8);
        auto w = new QLabel(name);
        w->setFont(AppConfig::get_instance().font(AppConfig::FontStyle::Light, 13));
        w->setAlignment(Qt::AlignLeft);
        auto pal1 = w->palette();
        pal1.setColor(QPalette::Window, Qt::transparent);
        pal1.setColor(QPalette::WindowText, QColor(15, 15, 15));
        w->setPalette(pal1);
        auto label = new QLabel(desc);
        label->setWordWrap(true);
        label->setFont(AppConfig::get_instance().font(AppConfig::FontStyle::Light, 10));
        label->setAlignment(Qt::AlignLeft);
        auto pal2 = label->palette();
        pal2.setColor(QPalette::Window, Qt::transparent);
        pal2.setColor(QPalette::WindowText, QColor(130, 130, 130));
        label->setPalette(pal2);
        layout->addWidget(w);
        layout->addWidget(label);
        return container;
    }

    QWidget *build_settings_item(const SettingsItem &item) {
        auto container        = new QWidget;
        auto layout_container = new QHBoxLayout(container);
        auto desc             = build_settings_desc(item.name, item.desc);
        layout_container->setContentsMargins({});
        layout_container->setSpacing(16);
        layout_container->addWidget(desc);
        layout_container->addWidget(item.widget);
        return container;
    }

    QWidget *build_group_item(const QString &name) {
        auto w = new QLabel(name);
        w->setFont(AppConfig::get_instance().font(AppConfig::FontStyle::Bold, 12));
        w->setContentsMargins({});
        auto pal = w->palette();
        pal.setColor(QPalette::WindowText, Qt::black);
        w->setPalette(pal);
        return w;
    }

    QWidget *build_item(const PanelItem &item) {
        return item.is_group_item ? build_group_item(std::any_cast<QString>(item.item))
                                  : build_settings_item(std::any_cast<SettingsItem>(item.item));
    }

    void add_separator_line() {
        auto sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        auto pal = sep->palette();
        pal.setColor(QPalette::WindowText, QColor(230, 230, 230));
        sep->setPalette(pal);
        layout_->addWidget(sep);
    }

public:
    void build() {
        bool prev_is_group = false;
        if (!items_.empty()) { layout_->addWidget(build_item(items_.takeFirst())); }
        for (auto &item : items_) {
            if (item.is_group_item) {
                layout_->addSpacing(32);
            } else {
                add_separator_line();
            }
            layout_->addWidget(build_item(item));
        }
    }

private:
    QList<PanelItem>   items_;
    QVBoxLayout *const layout_;
};

SettingsPanelBuilder &SettingsItemBuilder::complete() {
    parent_.commit(build());
    return parent_;
}

void SettingsDialog::handle_on_switch_to_panel(SettingsPanel panel) {
    if (auto w = ui_panel_area_->takeWidget()) {
        w->setParent(this);
        w->hide();
    }
    auto w = panels_[panel].panel;
    ui_panel_area_->setWidget(w);
    w->adjustSize();
}

int SettingsDialog::exec(OverlaySurface *surface) {
    Q_ASSERT(surface);
    surface->installEventFilter(this);
    const int result = OverlayDialog::exec(surface);
    surface->removeEventFilter(this);
    return result;
}

void SettingsDialog::show(OverlaySurface *surface) {
    auto dialog = std::make_unique<SettingsDialog>();
    dialog->exec(surface);
}

SettingsDialog::SettingsDialog()
    : OverlayDialog() {
    init();
}

void SettingsDialog::init() {
    auto layout = new QHBoxLayout(this);

    auto group_area    = new QScrollArea;
    auto settings_area = new QScrollArea;
    layout->addWidget(group_area);
    layout->addWidget(settings_area);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    auto pal1 = group_area->palette();
    pal1.setColor(QPalette::Window, QColor(245, 245, 245));
    group_area->setPalette(pal1);
    group_area->setWidgetResizable(true);
    group_area->setAutoFillBackground(true);
    group_area->setFrameShape(QFrame::NoFrame);
    group_area->setFixedWidth(200);

    auto pal2 = settings_area->palette();
    pal2.setColor(QPalette::Window, QColor(255, 255, 255));
    settings_area->setPalette(pal2);
    settings_area->setWidgetResizable(true);
    settings_area->setAutoFillBackground(true);
    settings_area->setFrameShape(QFrame::NoFrame);
    settings_area->setMinimumWidth(640);

    auto group        = new QWidget;
    auto group_layout = new QVBoxLayout(group);
    group_layout->setSpacing(8);
    group_layout->setContentsMargins(20, 0, 20, 0);

    auto pal3 = palette();
    pal3.setColor(QPalette::Window, QColor(255, 255, 255));
    setPalette(pal3);
    setAutoFillBackground(true);

    new_group(group_layout, "选项");
    panels_[SP_Normal].button     = new_settings_panel(group_layout, "常规");
    panels_[SP_Editor].button     = new_settings_panel(group_layout, "编辑");
    panels_[SP_Appearance].button = new_settings_panel(group_layout, "外观");
    panels_[SP_Backup].button     = new_settings_panel(group_layout, "备份");
    panels_[SP_Statistics].button = new_settings_panel(group_layout, "统计");
    panels_[SP_Shortcut].button   = new_settings_panel(group_layout, "热键");
    panels_[SP_About].button      = new_settings_panel(group_layout, "关于");

    new_group(group_layout, "高级");
    panels_[SP_Develop].button       = new_settings_panel(group_layout, "开发者选项");
    panels_[SP_DeepCustomize].button = new_settings_panel(group_layout, "深度定制");

    group_layout->addSpacing(24);
    group_layout->addStretch();

    auto group_scroll = new QtMaterialScrollBar;
    group_scroll->setHideOnMouseOut(false);
    group_scroll->setSliderColor(QColor(214, 214, 214));
    group_scroll->setBackgroundColor(QColor(245, 245, 245));
    group_area->setVerticalScrollBar(group_scroll);

    auto settings_scroll = new QtMaterialScrollBar;
    settings_scroll->setHideOnMouseOut(false);
    settings_scroll->setSliderColor(QColor(214, 214, 214));
    settings_scroll->setBackgroundColor(QColor(245, 245, 245));
    settings_area->setVerticalScrollBar(settings_scroll);

    group_area->setWidget(group);

    panels_[SP_Normal].panel        = createNormalPanel();
    panels_[SP_Editor].panel        = createEditorPanel();
    panels_[SP_Appearance].panel    = createAppearancePanel();
    panels_[SP_Backup].panel        = createBackupPanel();
    panels_[SP_Statistics].panel    = createStatisticsPanel();
    panels_[SP_Shortcut].panel      = createShortcutPanel();
    panels_[SP_About].panel         = createAboutPanel();
    panels_[SP_Develop].panel       = createDevelopPanel();
    panels_[SP_DeepCustomize].panel = createDeepCustomizePanel();

    ui_panel_area_ = settings_area;

    for (const auto type : magic_enum::enum_values<SettingsPanel>()) {
        panels_[type].panel->hide();
        connect(panels_[type].button, &FlatButton::pressed, [this, type] {
            handle_on_switch_to_panel(type);
        });
    }
    handle_on_switch_to_panel(SP_Normal);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    updateGeometry();
    update();
}

QWidget *SettingsDialog::createNormalPanel() {
    auto &config = AppConfig::get_instance();

    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_combo("语言", "界面语言")
        .with_item("简体中文", "zh_CN")
        .with_item("English", "en_US")
        .with_source(AppConfig::ValOption::Language)
        .complete()
        .with_combo(
            "默认视图",
            "应用启动时进入的默认视图\n指定编辑视图时，只写将自动定位到上一次退出时的编辑位置")
        .with_item("书库", "gallery")
        .with_item("编辑页面", "edit")
        .with_source(AppConfig::ValOption::PrimaryPage)
        .complete()
        .with_toggle("侧边栏隐藏", "全屏模式自动隐藏侧边栏")
        .with_source(AppConfig::Option::AutoHideToolbarOnFullscreen)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createEditorPanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_combo("默认编辑模式", "默认采用的编辑模式")
        .with_item("普通模式", "normal")
        .with_item("只读模式", "readonly")
        .with_source(AppConfig::ValOption::DefaultEditMode)
        .complete()
        .with_button("正文字体", "编辑器的正文字体候选列表")
        .with_text("选择")
        .with_signal(this, &SettingsDialog::on_request_editor_font_select)
        .complete()
        .with_spin("字体大小", "编辑视图字体大小")
        .with_bounds(8, 72)
        .with_step(1)
        .with_suffix("pt")
        .with_source(AppConfig::ValOption::TextFontSize)
        .complete()
        .with_toggle("首行缩进", "正文文本段落首行的缩进大小，默认为两个全角空格字符的大小")
        .with_source(AppConfig::Option::FirstLineIndent)
        .complete()
        .with_double_spin("行距", "正文文本使用的段落内行间距倍率")
        .with_bounds(0.0, 5.0)
        .with_step(0.2)
        .with_prefix("x")
        .with_source(AppConfig::ValOption::LineSpacingRatio)
        .complete()
        .with_double_spin("段落间距", "正文文本的段落间距")
        .with_bounds(0.0, 64.0)
        .with_step(1.0)
        .with_suffix("px")
        .with_source(AppConfig::ValOption::BlockSpacing)
        .complete()
        .with_toggle("段落聚焦", "高亮当前正在编辑的段落")
        .with_source(AppConfig::Option::HighlightActiveBlock)
        .complete()
        .with_toggle("弹性伸缩", "启用后，当页面宽度发生变化时，优先压缩边距以保持文本视图宽度不变")
        .with_source(AppConfig::Option::ElasticTextViewResize)
        .complete()
        .with_toggle("自动居中", "启用后，自动将编辑位置滚动到文本视图中心")
        .with_source(AppConfig::Option::CentreEditLine)
        .complete()
        .with_spin(
            "自动分章",
            "当字数超过设定值时，自动在新的章节编辑下一个段落，设定值为零时，禁用分章\n此动作只"
            "会在"
            "最后一个段落换行时触发")
        .with_bounds(0, 100000)
        .with_step(1000)
        .with_source(AppConfig::ValOption::AutoChapterThreshold)
        .complete()
        .with_toggle(
            "配对符号匹配",
            "启用该功能后，键入操作将在上下文双向匹配配对符号并触发增强编辑\n其中增强编辑包括："
            "键入"
            "符号修正、智能符号补全、符号配对删除、冗余符号过滤")
        .with_source(AppConfig::Option::PairingSymbolMatch)
        .complete()
        .with_combo("目录样式", "设置目录的显示风格")
        .with_item("树形模式", "tree")
        .with_item("平铺模式", "list")
        .with_source(AppConfig::ValOption::BookDirStyle)
        .complete()
        .with_spin(
            "文本告警阈限",
            "指定触发告警的文本阈限\n当单章字数以异常的高速状态超过阈限时，只写将立即抛出警告并"
            "冻结"
            "该章节的编辑权限，直到编辑者手动解冻\n该功能用于监测异常的超量文本输入，规避巨量数"
            "据冲"
            "击可能产生的应用卡顿、无响应乃至数据丢失")
        .with_bounds(0, 10000)
        .with_step(1)
        .with_suffix(" 万")
        .with_source(AppConfig::ValOption::CriticalChapterLimit)
        .complete()
        .with_spin(
            "单章字数上限",
            "指定单章的字数上限\n达到上限后将拒绝任何形式的文本插入，若指定了自动分章，则立即执"
            "行一"
            "次分章并将溢出的文本存入下一章节")
        .with_bounds(1, 1000)
        .with_step(1)
        .with_suffix(" 千")
        .with_source(AppConfig::ValOption::ChapterLimit)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createAppearancePanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_button("色彩方案", "主题与界面元素色彩配置")
        .with_text("设置")
        .with_signal(this, &SettingsDialog::on_request_color_scheme_editor)
        .complete()
        .with_button("背景图片", "设置应用的背景底图")
        .with_text("选择")
        .with_signal(this, &SettingsDialog::on_request_background_image_picker)
        .complete()
        .with_button("编辑背景", "设置编辑视图的背景底图\n未指定时，继承“背景图片”的配置")
        .with_text("选择")
        .with_signal(this, &SettingsDialog::on_request_editor_background_image_picker)
        .complete()
        .with_spin("背景不透明度", "应用背景图片的不透明度")
        .with_bounds(0, 100)
        .with_step(1)
        .with_suffix("%")
        .with_source(AppConfig::ValOption::BackgroundImageOpacity)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createBackupPanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_double_spin("定时备份", "指定定时备份的时间间隔")
        .with_bounds(0.5, 120)
        .with_step(1)
        .with_suffix("min")
        .with_source(AppConfig::ValOption::TimingBackupInterval)
        .complete()
        .with_spin(
            "定量备份", "指定定量备份的文本规模，当总编辑量达到指定值时，立即触发一次备份动作")
        .with_bounds(50, 5000)
        .with_step(100)
        .with_source(AppConfig::ValOption::QuantitativeBackupThreshold)
        .complete()
        .with_toggle("智能压缩", "启用后，将尝试合并连续的零碎备份记录")
        .with_source(AppConfig::Option::BackupSmartMerge)
        .complete()
        .with_toggle(
            "关键版本识别",
            "启用后，将根据历史编辑动作识别当前文档关键编辑节点，并生成独立的备份记录")
        .with_source(AppConfig::Option::KeyVersionRecognition)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createStatisticsPanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_toggle(
            "严格字数统计",
            "启用后，将忽略对标点符号等非字母符号的计数，并将数字、英文单词视作单个字符统计")
        .with_source(AppConfig::Option::StrictWordCount)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createShortcutPanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_group("通用")
        .with_shortcut("全屏模式", "切换/退出全屏模式")
        .with_action(GlobalCommand::ToggleFullscreen)
        .complete()
        .with_shortcut("目录显示", "显示/隐藏作品目录")
        .with_action(GlobalCommand::ToggleSidebar)
        .complete()
        .with_shortcut("软居中模式", "切换/退出软居中模式")
        .with_action(GlobalCommand::ToggleSoftCenterMode)
        .complete()
        .with_shortcut("创建新章节", "在当前卷的末尾创建新章节")
        .with_action(GlobalCommand::CreateNewChapter)
        .complete()
        .with_shortcut("重命名", "在编辑视图时，重命名当前选中的目录项（卷名、章名）")
        .with_action(GlobalCommand::Rename)
        .complete()
        .with_group("文本编辑")
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createAboutPanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_button("应用版本", QString("当前版本：v%1").arg(jwrite::VERSION.toString()))
        .with_text("检查更新")
        .with_signal(this, &SettingsDialog::on_request_check_update)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createDevelopPanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);
    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QWidget *SettingsDialog::createDeepCustomizePanel() {
    auto w      = new QWidget;
    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(54, 0, 54, 0);
    layout->setSpacing(8);
    layout->addSpacing(32);

    SettingsPanelBuilder(layout)
        .with_spin("侧边栏图标大小", "指定侧边栏工具图标的大小\n该设置不会改变与侧边栏边框的间距")
        .with_bounds(8, 64)
        .with_step(1)
        .with_suffix("px")
        .with_source(AppConfig::ValOption::ToolbarIconSize)
        .complete()
        .build();

    layout->addSpacing(32);
    layout->addStretch();
    return w;
}

QSize SettingsDialog::minimumSizeHint() const {
    const QSize min_size(528, 225);
    const int   min_margin = 20;
    return min_size + QSize(2 * min_margin, 2 * min_margin);
}

QSize SettingsDialog::sizeHint() const {
    const auto parent = surface();
    if (!parent) { return minimumSizeHint(); }

    const QSize min_size(528, 225);
    const QSize max_size(1100, 700);

    const auto max_bb = parent->contentsRect();

    const int left_width  = max_bb.width() - min_size.width();
    const int left_height = max_bb.height() - min_size.height();

    const int width =
        qBound<int>(min_size.width(), min_size.width() + left_width * 0.8, max_size.width());
    const int height =
        qBound<int>(min_size.height(), min_size.height() + left_height * 0.8, max_size.height());

    return QSize(width, height);
}

bool SettingsDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        const auto e   = static_cast<QMouseEvent *>(event);
        const auto pos = mapFromGlobal(e->globalPosition()).toPoint();
        if (!contentsRect().contains(pos)) { quit(); }
    }
    return false;
}

} // namespace jwrite::ui
