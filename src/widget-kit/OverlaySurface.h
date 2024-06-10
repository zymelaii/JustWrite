#pragma once

#include <QWidget>
#include <QHBoxLayout>

namespace widgetkit {

class OverlayDialog;

class OverlaySurface : public QWidget {
    Q_OBJECT

public:
    OverlaySurface();
    ~OverlaySurface() override;

    bool setup(QWidget *widget);

    void setColor(const QColor &color);
    void setOpacity(double opacity);

    void reload(OverlayDialog *overlay);

    OverlayDialog *overlay() const {
        return overlay_;
    }

public slots:
    bool showOverlay();
    void closeOverlay();

protected:
    void setupUi();
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    OverlayDialog *overlay_;
    QWidget       *ui_widget_;
    QColor         ui_color_;
    int            ui_opacity_;
    QHBoxLayout   *ui_layout_;
};

} // namespace widgetkit
