#pragma once

#include <jwrite/BookManager.h>
#include <qt-material/qtmaterialtextfield.h>
#include <qt-material/qtmaterialraisedbutton.h>
#include <QWidget>
#include <QLabel>

namespace jwrite::ui {

class BookInfoEdit : public QWidget {
    Q_OBJECT

public:
    explicit BookInfoEdit(QWidget *parent = nullptr);
    ~BookInfoEdit() override;

public:
    void setTitle(const QString &title);
    void setAuthor(const QString &author);
    void setCover(const QString &cover_url);
    void setBookInfo(const BookInfo &info);

signals:
    void submitRequested(BookInfo info);
    void cancelRequested();
    void changeCoverRequested();

protected:
    void setupUi();
    void setupConnections();

    void paintEvent(QPaintEvent *event) override;

private:
    BookInfo book_info_;

    QtMaterialTextField    *ui_title_edit_;
    QtMaterialTextField    *ui_author_edit_;
    QLabel                 *ui_cover_;
    QtMaterialRaisedButton *ui_cover_select_;
    QtMaterialRaisedButton *ui_submit_;
    QtMaterialRaisedButton *ui_cancel_;
};

} // namespace jwrite::ui
