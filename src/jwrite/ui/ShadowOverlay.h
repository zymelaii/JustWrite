#pragma once

#include <QWidget>
#include <QHBoxLayout>

namespace jwrite::ui {

class ShadowOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ShadowOverlay(QWidget* parent = nullptr);
    ~ShadowOverlay() override;

public:
    QWidget* take();
    QWidget* setWidget(QWidget* widget);

protected:
    void setupUi();

    void paintEvent(QPaintEvent* event) override;

private:
    QVBoxLayout* ui_layout_;
};

}; // namespace jwrite::ui
