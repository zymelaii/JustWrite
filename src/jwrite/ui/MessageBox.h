#pragma once

#include <widget-kit/FlatButton.h>
#include <widget-kit/ClickableLabel.h>
#include <widget-kit/OverlayDialog.h>
#include <QLabel>
#include <optional>

namespace jwrite::ui {

class MessageBox : public widgetkit::OverlayDialog {
    Q_OBJECT

public:
    enum Choice {
        Cancel,
        Yes,
        No,
    };

    enum class Type {
        Notify,
        Confirm,
        Ternary,
    };

    enum class StandardIcon {
        Info,
        Warning,
        Error,
    };

    struct Option {
        Type                        type;
        std::optional<StandardIcon> icon;
        std::optional<QString>      icon_path;
        std::optional<QString>      cancel_text;
        std::optional<QString>      yes_text;
        std::optional<QString>      no_text;
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

    void setType(Type type);

    void setButtonText(Choice choice, const QString &text);

    static Choice show(
        widgetkit::OverlaySurface *surface,
        const QString             &caption,
        const QString             &text,
        const Option              &option);

    static Choice
        show(widgetkit::OverlaySurface *surface, const QString &caption, const QString &text);

    static Choice show(
        widgetkit::OverlaySurface *surface,
        const QString             &caption,
        const QString             &text,
        StandardIcon               icon);

    static QString standardIconPath(StandardIcon icon);

protected:
    void setupUi();
    void setupConnections();

    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLabel                    *ui_caption_;
    widgetkit::ClickableLabel *ui_close_;
    QLabel                    *ui_icon_;
    QLabel                    *ui_message_;
    widgetkit::FlatButton     *ui_btn_cancel_;
    widgetkit::FlatButton     *ui_btn_no_;
    widgetkit::FlatButton     *ui_btn_yes_;
};

} // namespace jwrite::ui
