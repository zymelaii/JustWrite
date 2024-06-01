#pragma once

#include <jwrite/BookManager.h>
#include <widget-kit/FlatButton.h>
#include <widget-kit/ImageLabel.h>
#include <widget-kit/OverlayDialog.h>
#include <qt-material/qtmaterialtextfield.h>
#include <QWidget>

namespace jwrite::ui {

class BookInfoEdit : public widgetkit::OverlayDialog {
    Q_OBJECT

public:
    void set_title(const QString &title);
    void set_author(const QString &author);
    void set_cover(const QString &cover_url);
    void set_book_info(const BookInfo &info);

    static QString select_cover(QWidget *parent, bool validate, QImage *out_image);

    static std::optional<BookInfo>
        get_book_info(widgetkit::OverlaySurface *surface, const BookInfo &initial);

protected slots:
    void handle_on_select_cover();
    void handle_on_submit();

public:
    BookInfoEdit();

protected:
    void init();

    void paintEvent(QPaintEvent *event) override;

private:
    BookInfo book_info_;

    QtMaterialTextField   *ui_title_edit_;
    QtMaterialTextField   *ui_author_edit_;
    widgetkit::ImageLabel *ui_cover_;
    widgetkit::FlatButton *ui_cover_select_;
    widgetkit::FlatButton *ui_submit_;
    widgetkit::FlatButton *ui_cancel_;
};

} // namespace jwrite::ui
