#pragma once

#include <qt-material/qtmaterialtextfield.h>
#include <QWidget>
#include <QPushButton>

namespace jwrite::ui {

class BookInfoEdit : public QWidget {
    Q_OBJECT

public:
    explicit BookInfoEdit(QWidget *parent = nullptr);
    ~BookInfoEdit() override;

protected:
    void setupUi();

    void paintEvent(QPaintEvent *event) override;

private:
    QtMaterialTextField *ui_title_edit_;
    QtMaterialTextField *ui_author_edit_;
    QPushButton         *ui_cover_select_;
};

} // namespace jwrite::ui
