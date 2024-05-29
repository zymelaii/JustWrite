#include <jwrite/ui/TitleBar.h>
#include <QPainter>
#include <QHBoxLayout>

namespace jwrite::ui {

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent) {
    setupUi();
    setupConnections();
}

TitleBar::~TitleBar() {}

QString TitleBar::title() const {
    return title_;
}

void TitleBar::setTitle(const QString &title) {
    title_ = title;
    update();
}

SystemButton *TitleBar::button(SystemButton::SystemCommand command_type) const {
    return sys_buttons_[magic_enum::enum_index(command_type).value()];
}

void TitleBar::updateColorScheme(const ColorScheme &scheme) {
    for (auto &button : sys_buttons_) { button->reloadIcon(); }
}

void TitleBar::requestMinimize() {
    emit minimizeRequested();
}

void TitleBar::requestMaximize() {
    emit maximizeRequested();
}

void TitleBar::requestClose() {
    emit closeRequested();
}

QSize TitleBar::minimumSizeHint() const {
    const auto margins      = contentsMargins();
    const auto fm           = fontMetrics();
    const int  hori_spacing = 4;
    const int  height       = fm.height() + fm.descent() + margins.top() + margins.bottom();
    return QSize(0, height + hori_spacing * 2);
}

QSize TitleBar::sizeHint() const {
    return minimumSizeHint();
}

void TitleBar::setupUi() {
    auto layout = new QHBoxLayout(this);

    layout->addStretch();

    for (const auto type : magic_enum::enum_values<SystemButton::SystemCommand>()) {
        auto button = new SystemButton(type);
        layout->addWidget(button);
        sys_buttons_[magic_enum::enum_index(type).value()] = button;
    }

    layout->setContentsMargins({});
    layout->setSpacing(0);

    auto font = this->font();
    font.setPointSize(10);
    setFont(font);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void TitleBar::setupConnections() {
    connect(
        sys_buttons_[magic_enum::enum_index<SystemButton::Minimize>()],
        &SystemButton::clicked,
        this,
        &TitleBar::requestMinimize);
    connect(
        sys_buttons_[magic_enum::enum_index<SystemButton::Maximize>()],
        &SystemButton::clicked,
        this,
        &TitleBar::requestMaximize);
    connect(
        sys_buttons_[magic_enum::enum_index<SystemButton::Close>()],
        &SystemButton::clicked,
        this,
        &TitleBar::requestClose);
}

void TitleBar::paintEvent(QPaintEvent *event) {
    QPainter   p(this);
    const auto pal = palette();

    p.fillRect(rect(), pal.window());

    p.setPen(pal.windowText().color());
    p.drawText(contentsRect(), Qt::AlignCenter, title_);
}

} // namespace jwrite::ui
