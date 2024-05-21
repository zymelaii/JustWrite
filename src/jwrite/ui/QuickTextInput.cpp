#include <jwrite/ui/QuickTextInput.h>
#include <QVBoxLayout>
#include <QPainter>
#include <QKeyEvent>

namespace jwrite::ui {

QuickTextInput::QuickTextInput(QWidget *parent)
    : QWidget(parent) {
    setupUi();
    setFocusProxy(ui_input_);
    ui_input_->installEventFilter(this);
}

QuickTextInput::~QuickTextInput() {}

void QuickTextInput::setText(const QString &text) {
    ui_input_->setText(text);
}

QString QuickTextInput::text() const {
    return ui_input_->text();
}

void QuickTextInput::setPlaceholderText(const QString &text) {
    ui_input_->setPlaceholderText(text);
}

void QuickTextInput::setLabel(const QString &label) {
    ui_input_->setLabel(label);
}

void QuickTextInput::setupUi() {
    auto layout = new QVBoxLayout(this);

    ui_input_ = new QtMaterialTextField;

    layout->addWidget(ui_input_);

    layout->setContentsMargins(32, 32, 32, 32);

    auto pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Text, Qt::black);
    setPalette(pal);

    auto font = this->font();

    ui_input_->setFont(font);
    ui_input_->setLabelFontSize(12);

    setFixedSize(480, 150);
}

void QuickTextInput::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

bool QuickTextInput::eventFilter(QObject *watched, QEvent *event) {
    if (watched != ui_input_ || !ui_input_->hasFocus() || event->type() == QEvent::KeyPress) {
        return false;
    }

    const auto key = static_cast<QKeyEvent *>(event)->key();

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        emit submitRequested(ui_input_->text());
        return true;
    }
    if (key == Qt::Key_Escape) {
        emit cancelRequested();
        return true;
    }

    return false;
}

} // namespace jwrite::ui
