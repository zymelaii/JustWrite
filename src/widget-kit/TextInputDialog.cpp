#include <widget-kit/TextInputDialog.h>
#include <qt-material/qtmaterialtextfield.h>
#include <QVBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <memory>

namespace widgetkit {

TextInputDialog::TextInputDialog()
    : OverlayDialog() {
    setupUi();
    setFocusProxy(ui_input_);
    ui_input_->installEventFilter(this);
}

TextInputDialog::~TextInputDialog() {}

void TextInputDialog::setCaption(const QString &label) {
    ui_input_->setLabel(label);
}

void TextInputDialog::setPlaceholderText(const QString &text) {
    ui_input_->setPlaceholderText(text);
}

void TextInputDialog::setText(const QString &text) {
    ui_input_->setText(text);
}

QString TextInputDialog::text() const {
    return ui_input_->text();
}

std::optional<QString> TextInputDialog::getInputText(
    OverlaySurface *surface,
    const QString  &initial,
    const QString  &caption,
    const QString  &placeholder) {
    auto dialog = std::make_unique<TextInputDialog>();
    dialog->setCaption(caption);
    dialog->setText(initial);
    dialog->setPlaceholderText(placeholder);
    dialog->exec(surface);
    return dialog->isAccepted() ? std::make_optional(dialog->text()) : std::nullopt;
}

void TextInputDialog::setupUi() {
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

    setFixedWidth(480);
}

void TextInputDialog::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

bool TextInputDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched != ui_input_ || !ui_input_->hasFocus() || event->type() == QEvent::KeyPress) {
        return false;
    }

    const auto key = static_cast<QKeyEvent *>(event)->key();

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        accept();
        return true;
    }
    if (key == Qt::Key_Escape) {
        reject();
        return true;
    }

    return false;
}

} // namespace widgetkit
