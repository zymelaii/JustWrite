#pragma once

#include <QWidget>
#include <QGridLayout>

namespace jwrite::ui {

class FloatingDialog : public QWidget {
    Q_OBJECT

public:
    explicit FloatingDialog(QWidget* parent = nullptr);
    ~FloatingDialog() override;

public:
    QWidget* take();
    QWidget* setWidget(QWidget* widget);

protected:
    void setupUi();

    void paintEvent(QPaintEvent* event) override;

private:
    QGridLayout* ui_layout_;
};

}; // namespace jwrite::ui
