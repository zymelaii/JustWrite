#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>

namespace jwrite::ui {

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);
    virtual ~StatusBar();

public:
    void    setSpacing(int spacing);
    QLabel *addItem(const QString &text, bool right_side);

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

private:
    int total_left_side_items_;

    QHBoxLayout *ui_layout_;
};

} // namespace jwrite::ui
