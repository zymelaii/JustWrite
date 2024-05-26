#pragma once

#include <widget-kit/OverlayDialog.h>

class QtMaterialTextField;

namespace widgetkit {

class TextInputDialog : public OverlayDialog {
    Q_OBJECT

public:
    TextInputDialog();
    ~TextInputDialog() override;

public:
    void setCaption(const QString &caption);
    void setPlaceholderText(const QString &text);
    void setText(const QString &text);

    QString text() const;

    static std::optional<QString> getInputText(
        OverlaySurface *surface,
        const QString  &initial     = QString(),
        const QString  &caption     = QString(),
        const QString  &placeholder = QString());

protected:
    void setupUi();

    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QtMaterialTextField *ui_input_;
};

} // namespace widgetkit
