#include <jwrite/ui/MessageBox.h>
#include <widget-kit/FlatButton.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QKeyEvent>
#include <QEventLoop>
#include <memory>

namespace widgetkit {

MessageBox::MessageBox()
    : OverlayDialog() {
    setupUi();
    setupConnections();
}

MessageBox::~MessageBox() {}

void MessageBox::setupUi() {
    ui_caption_ = new QLabel;
    ui_close_   = new ClickableLabel;
    ui_icon_    = new QLabel;
    ui_message_ = new QLabel;
    ui_btn_no_  = new FlatButton;
    ui_btn_yes_ = new FlatButton;

    auto caption_container = new QWidget;
    auto icon_container    = new QWidget;
    auto message_container = new QWidget;
    auto button_container  = new QWidget;

    auto layout         = new QVBoxLayout(this);
    auto layout_caption = new QHBoxLayout(caption_container);
    auto layout_message = new QHBoxLayout(message_container);
    auto layout_button  = new QHBoxLayout(button_container);

    layout->addWidget(caption_container);
    layout->addWidget(message_container);
    layout->addWidget(button_container);

    layout_caption->addWidget(ui_caption_);
    layout_caption->addStretch();
    layout_caption->addWidget(ui_close_);

    layout_message->addWidget(ui_icon_);
    layout_message->addWidget(ui_message_);

    layout_button->addStretch();
    layout_button->addWidget(ui_btn_no_);
    layout_button->addWidget(ui_btn_yes_);

    layout->setContentsMargins({});
    layout_caption->setContentsMargins({});
    layout_message->setContentsMargins({});
    layout_button->setContentsMargins({});

    layout->setSpacing(16);
    layout_button->setSpacing(8);

    ui_caption_->setText("提示");
    ui_close_->setPixmap(QIcon(":/res/icons/close-large.svg").pixmap(QSize(16, 16)));
    ui_message_->setText("");
    ui_btn_no_->setText("取消");
    ui_btn_yes_->setText("确定");

    auto font = this->font();
    font.setPixelSize(20);
    ui_caption_->setFont(font);
    font.setPixelSize(16);
    ui_message_->setFont(font);
    font.setPixelSize(14);
    ui_btn_no_->setFont(font);
    ui_btn_yes_->setFont(font);

    auto pal = palette();
    pal.setColor(QPalette::WindowText, QColor("#38393a"));
    ui_caption_->setPalette(pal);
    pal.setColor(QPalette::WindowText, QColor("#636569"));
    ui_message_->setPalette(pal);
    pal.setColor(QPalette::Window, QColor("#a2a2a2"));
    pal.setColor(QPalette::Base, QColor("#f3f3f3"));
    pal.setColor(QPalette::Button, QColor("#e3e3e3"));
    pal.setColor(QPalette::WindowText, QColor("#787a7d"));
    ui_btn_no_->setPalette(pal);
    pal.setColor(QPalette::Base, QColor("#65696c"));
    pal.setColor(QPalette::Button, QColor("#85898c"));
    pal.setColor(QPalette::WindowText, QColor("#ffffff"));
    ui_btn_yes_->setPalette(pal);

    ui_message_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    ui_message_->setWordWrap(true);

    ui_btn_no_->setBorderVisible(true);
    ui_btn_yes_->setBorderVisible(true);

    ui_btn_no_->setRadius(4);
    ui_btn_yes_->setRadius(4);

    ui_btn_no_->setContentsMargins(20, 4, 20, 4);
    ui_btn_yes_->setContentsMargins(20, 4, 20, 4);

    ui_icon_->setVisible(false);

    setContentsMargins(20, 16, 20, 16);

    setFixedWidth(400);
}

void MessageBox::setupConnections() {
    connect(ui_close_, &ClickableLabel::clicked, this, &OverlayDialog::reject);
    connect(ui_btn_no_, &FlatButton::pressed, this, &OverlayDialog::reject);
    connect(ui_btn_yes_, &FlatButton::pressed, this, &OverlayDialog::accept);
}

void MessageBox::setIcon(const QString &icon_path) {
    QIcon icon(icon_path);
    if (icon.isNull()) { return; }
    ui_icon_->setPixmap(icon.pixmap(QSize(24, 24)));
    ui_icon_->setVisible(true);
}

MessageBox::Choice MessageBox::show(
    OverlaySurface *surface,
    const QString  &caption,
    const QString  &text,
    const QString  &icon_path) {
    auto dialog = std::make_unique<MessageBox>();
    dialog->setCaption(caption);
    dialog->setText(text);
    if (!icon_path.isEmpty()) { dialog->setIcon(icon_path); }
    dialog->exec(surface);
    const auto choice = dialog->isAccepted() ? Choice::Yes
                      : dialog->isRejected() ? Choice::No
                                             : Choice::Cancel;
    Q_ASSERT(choice != Choice::Cancel);
    return choice;
}

void MessageBox::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(Qt::white);
    p.setBrush(Qt::white);

    p.drawRoundedRect(rect(), 12, 12);
}

void MessageBox::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) { reject(); }
}

} // namespace widgetkit
