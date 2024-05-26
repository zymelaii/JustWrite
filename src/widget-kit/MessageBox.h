#pragma once

#include <widget-kit/FlatButton.h>
#include <widget-kit/ClickableLabel.h>
#include <widget-kit/OverlayDialog.h>
#include <QLabel>

namespace widgetkit {

class MessageBox : public OverlayDialog {
    Q_OBJECT

public:
    enum Choice {
        Yes,
        No,
        Cancel,
    };

public:
    MessageBox();
    ~MessageBox() override;

public:
    void setCaption(const QString &caption) {
        ui_caption_->setText(caption);
    }

    void setText(const QString &message) {
        ui_message_->setText(message);
    }

    void setIcon(const QString &icon_path);

    static Choice show(
        OverlaySurface *surface,
        const QString  &caption,
        const QString  &text,
        const QString  &icon_path = QString());

protected:
    void setupUi();
    void setupConnections();

    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLabel         *ui_caption_;
    ClickableLabel *ui_close_;
    QLabel         *ui_icon_;
    QLabel         *ui_message_;
    FlatButton     *ui_btn_no_;
    FlatButton     *ui_btn_yes_;
};

} // namespace widgetkit
