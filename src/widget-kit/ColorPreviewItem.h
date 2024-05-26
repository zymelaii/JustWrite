#pragma once

#include <widget-kit/ClickableLabel.h>
#include <QSharedPointer>

namespace widgetkit {

class ColorPickerDelegate {
public:
    virtual QColor getColor(
        const QColor  &init_color = Qt::white,
        QWidget       *parent     = nullptr,
        const QString &title      = QString()) = 0;
};

class ColorPreviewItem : public ClickableLabel {
    Q_OBJECT

public:
    explicit ColorPreviewItem(QWidget *parent = nullptr);

    ~ColorPreviewItem() override;

signals:
    void colorChanged(QColor color);

public:
    QColor color() const {
        return color_;
    }

    void setColor(const QColor &color);

    void setDelegate(QSharedPointer<ColorPickerDelegate> delegate) {
        delegate_ = delegate;
    }

private slots:
    void showColorPicker();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor                              color_;
    QSharedPointer<ColorPickerDelegate> delegate_;
};

} // namespace widgetkit
