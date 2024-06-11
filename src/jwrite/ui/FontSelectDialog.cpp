#include <jwrite/ui/FontSelectDialog.h>
#include <jwrite/AppConfig.h>
#include <widget-kit/ClickableLabel.h>
#include <widget-kit/ElidedLabel.h>
#include <qt-material/qtmaterialscrollbar.h>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QPainter>
#include <QScrollArea>
#include <QAbstractItemView>
#include <QScreen>
#include <memory>

using namespace widgetkit;

namespace jwrite::ui {

void FontSelectDialog::add_font(const QString &family) {
    if (font_families_.contains(family)) { return; }
    font_families_.append(family);

    auto &config = AppConfig::get_instance();

    const bool valid = QFontDatabase::hasFamily(family);

    auto container  = new QWidget;
    auto layout     = new QHBoxLayout(container);
    auto state      = new QLabel;
    auto font_label = new ElidedLabel;
    auto up         = new ClickableLabel;
    auto down       = new ClickableLabel;
    auto remove     = new ClickableLabel;
    layout->addWidget(state);
    layout->addWidget(font_label);
    layout->addStretch();
    layout->addWidget(up);
    layout->addWidget(down);
    layout->addWidget(remove);
    layout->setContentsMargins({});

    const int font_size = 16;
    font_label->setText(family);
    auto font = valid ? QFont(family) : font_label->font();
    font.setPointSize(font_size);
    font_label->setFont(font);
    auto pal = font_label->palette();
    pal.setColor(QPalette::WindowText, Qt::black);
    font_label->setPalette(pal);
    font_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    const double button_extent = 12 * screen()->devicePixelRatio();
    const double state_extent  = 16 * screen()->devicePixelRatio();

#define JWRITE_SPECIFY_ICON(w, icon_name, extent) \
    w->setPixmap(                                 \
        QPixmap(config.icon((icon_name)))         \
            .scaled(extent, extent, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    JWRITE_SPECIFY_ICON(state, valid ? "sign/done" : "sign/warning", state_extent);
    JWRITE_SPECIFY_ICON(up, "button/up", button_extent);
    JWRITE_SPECIFY_ICON(down, "button/down", button_extent);
    JWRITE_SPECIFY_ICON(remove, "button/close", button_extent);

#undef JWRITE_SPECIFY_ICON

    container->setFixedHeight(28);

    ui_font_layout_->insertWidget(ui_font_layout_->count() - 1, container);
    ui_font_layout_->parentWidget()->adjustSize();

    connect(up, &ClickableLabel::clicked, [this, container] {
        const int index = ui_font_layout_->indexOf(container);
        Q_ASSERT(index >= 0 && index < ui_font_layout_->count() - 1);
        if (index > 0 && ui_font_layout_->count() > 2) {
            font_families_.insert(index - 1, font_families_.takeAt(index));
            ui_font_layout_->takeAt(index);
            ui_font_layout_->insertWidget(index - 1, container);
            ui_font_layout_->parentWidget()->adjustSize();
        }
    });

    connect(down, &ClickableLabel::clicked, [this, container] {
        const int index = ui_font_layout_->indexOf(container);
        Q_ASSERT(index >= 0 && index < ui_font_layout_->count() - 1);
        if (index < ui_font_layout_->count() - 2 && ui_font_layout_->count() > 2) {
            font_families_.insert(index + 1, font_families_.takeAt(index));
            ui_font_layout_->takeAt(index);
            ui_font_layout_->insertWidget(index + 1, container);
            ui_font_layout_->parentWidget()->adjustSize();
        }
    });

    connect(remove, &ClickableLabel::clicked, [this, container] {
        const int index = ui_font_layout_->indexOf(container);
        Q_ASSERT(index >= 0 && index < ui_font_layout_->count() - 1);
        font_families_.removeAt(index);
        ui_font_layout_->takeAt(index)->widget()->deleteLater();
        ui_font_layout_->parentWidget()->adjustSize();
    });
}

std::optional<QStringList>
    FontSelectDialog::get_font_families(OverlaySurface *surface, QStringList initial_families) {
    auto dialog = std::make_unique<FontSelectDialog>();
    for (auto family : initial_families) {
        if (family.isEmpty()) { continue; }
        dialog->add_font(family);
    }
    dialog->exec(surface);
    if (dialog->isAccepted()) {
        return {dialog->font_families_};
    } else {
        return std::nullopt;
    }
}

FontSelectDialog::FontSelectDialog()
    : OverlayDialog() {
    init();
}

FontSelectDialog::~FontSelectDialog() {}

void FontSelectDialog::init() {
    auto label = new QLabel;

    auto combo_container = new QWidget;
    auto combo_layout    = new QHBoxLayout(combo_container);
    ui_font_combo_       = new QComboBox;
    ui_commit_           = new FlatButton;
    combo_layout->addWidget(ui_font_combo_);
    combo_layout->addWidget(ui_commit_);
    combo_layout->setContentsMargins({});

    auto button_container = new QWidget;
    auto button_layout    = new QHBoxLayout(button_container);
    auto accept_button    = new FlatButton;
    auto cancel_button    = new FlatButton;
    button_layout->addStretch();
    button_layout->addWidget(accept_button);
    button_layout->addWidget(cancel_button);
    button_layout->setContentsMargins({});

    auto fonts_scroll = new QtMaterialScrollBar;
    fonts_scroll->setHideOnMouseOut(false);
    fonts_scroll->setSliderColor(QColor(214, 214, 214));
    fonts_scroll->setBackgroundColor(QColor(245, 245, 245));
    auto fonts_area = new QScrollArea;
    fonts_area->setWidgetResizable(true);
    fonts_area->setAutoFillBackground(false);
    fonts_area->setFrameShape(QFrame::NoFrame);
    fonts_area->setFixedHeight(200);
    fonts_area->setVerticalScrollBar(fonts_scroll);
    fonts_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fonts_area->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto font_families = new QWidget;
    ui_font_layout_    = new QVBoxLayout(font_families);
    ui_font_layout_->addStretch();
    ui_font_layout_->setSpacing(12);
    fonts_area->setWidget(font_families);

    auto separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    auto pal = separator->palette();
    pal.setColor(QPalette::WindowText, QColor(230, 230, 230));
    separator->setPalette(pal);

    ui_layout_ = new QVBoxLayout(this);
    ui_layout_->addWidget(label);
    ui_layout_->addWidget(fonts_area);
    ui_layout_->addWidget(separator);
    ui_layout_->addWidget(combo_container);
    ui_layout_->addWidget(button_container);
    ui_layout_->setSpacing(12);
    ui_layout_->setContentsMargins(32, 32, 32, 32);

    auto pal1 = palette();
    pal1.setColor(QPalette::Window, Qt::white);
    pal1.setColor(QPalette::Base, Qt::white);
    pal1.setColor(QPalette::Text, Qt::black);
    setPalette(pal1);

    label->setText(
        "列表中的字体将被按次序使用。若当前字体不存在或字形缺失，则顺延至列表中的下一个字体。");
    label->setWordWrap(true);
    auto font1 = font();
    font1.setPointSize(12);
    label->setFont(font1);
    auto pal2 = label->palette();
    pal2.setColor(QPalette::WindowText, QColor("#000000"));
    label->setPalette(pal2);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    auto combo_scroll = new QtMaterialScrollBar;
    combo_scroll->setHideOnMouseOut(false);
    combo_scroll->setSliderColor(QColor(214, 214, 214));
    combo_scroll->setBackgroundColor(QColor(245, 245, 245));
    ui_font_combo_->view()->setVerticalScrollBar(combo_scroll);
    for (auto family : QFontDatabase::families()) { ui_font_combo_->addItem(family); }
    ui_font_combo_->setCurrentIndex(-1);
    ui_font_combo_->setPlaceholderText("选择字体");
    ui_font_combo_->setStyleSheet(R"(
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
    ui_font_combo_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    auto font2 = font();
    font2.setPixelSize(14);

    ui_commit_->setText("添加");
    ui_commit_->setRadius(4);
    ui_commit_->setBorderVisible(true);
    ui_commit_->setTextAlignment(Qt::AlignCenter);
    ui_commit_->setContentsMargins(20, 4, 20, 4);
    ui_commit_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui_commit_->setFont(font2);
    auto pal3 = ui_commit_->palette();
    pal3.setColor(QPalette::Window, QColor("#a2a2a2"));
    pal3.setColor(QPalette::Base, QColor("#65696c"));
    pal3.setColor(QPalette::Button, QColor("#85898c"));
    pal3.setColor(QPalette::WindowText, QColor("#ffffff"));
    ui_commit_->setPalette(pal3);

    accept_button->setText("确定");
    accept_button->setRadius(4);
    accept_button->setBorderVisible(true);
    accept_button->setTextAlignment(Qt::AlignCenter);
    accept_button->setContentsMargins(20, 4, 20, 4);
    accept_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    accept_button->setFont(font2);
    accept_button->setPalette(pal3);

    cancel_button->setText("取消");
    cancel_button->setRadius(4);
    cancel_button->setBorderVisible(true);
    cancel_button->setTextAlignment(Qt::AlignCenter);
    cancel_button->setContentsMargins(20, 4, 20, 4);
    cancel_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cancel_button->setFont(font2);
    auto pal4 = cancel_button->palette();
    pal4.setColor(QPalette::Window, QColor("#a2a2a2"));
    pal4.setColor(QPalette::Base, QColor("#f3f3f3"));
    pal4.setColor(QPalette::Button, QColor("#e3e3e3"));
    pal4.setColor(QPalette::WindowText, QColor("#787a7d"));
    cancel_button->setPalette(pal4);

    setFixedWidth(480);

    connect(ui_commit_, &FlatButton::pressed, this, [this] {
        if (ui_font_combo_->currentIndex() == -1) { return; }
        const auto family = ui_font_combo_->currentText();
        add_font(family);
        ui_font_combo_->setCurrentIndex(-1);
    });

    connect(accept_button, &FlatButton::pressed, this, &FontSelectDialog::accept);
    connect(cancel_button, &FlatButton::pressed, this, &FontSelectDialog::reject);
}

void FontSelectDialog::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

} // namespace jwrite::ui
