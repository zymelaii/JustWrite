#pragma once

#include <widget-kit/ClickableLabel.h>
#include <QWidget>
#include <QAction>

namespace jwrite::ui {

class FloatingLabel : public widgetkit::ClickableLabel {
    Q_OBJECT

public:
    void set_text(const QString& text);
    void update_geometry();

public:
    explicit FloatingLabel(QWidget* parent);
    ~FloatingLabel() override;

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

    bool eventFilter(QObject* watched, QEvent* event) override;
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
};

} // namespace jwrite::ui
