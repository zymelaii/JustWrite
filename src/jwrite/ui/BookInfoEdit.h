#pragma once

#include <jwrite/BookManager.h>
#include <widget-kit/FlatButton.h>
#include <widget-kit/ImageLabel.h>
#include <widget-kit/OverlayDialog.h>
#include <qt-material/qtmaterialtextfield.h>
#include <qt-material/qtmaterialraisedbutton.h>
#include <QWidget>
#include <QLabel>

namespace jwrite::ui {

class BookInfoEdit : public widgetkit::OverlayDialog {
    Q_OBJECT

public:
    enum Request {
        Submit,
        Cancel,
    };

public:
    BookInfoEdit();
    ~BookInfoEdit() override;

public:
    void setTitle(const QString &title);
    void setAuthor(const QString &author);
    void setCover(const QString &cover_url);
    void setBookInfo(const BookInfo &info);

    BookInfoEdit &withTitle(const QString &title) {
        setTitle(title);
        return *this;
    }

    BookInfoEdit &withAuthor(const QString &author) {
        setAuthor(author);
        return *this;
    }

    BookInfoEdit &withCover(const QString &cover_url) {
        setCover(cover_url);
        return *this;
    }

    BookInfoEdit &withBookInfo(const BookInfo &info) {
        setBookInfo(info);
        return *this;
    }

    const BookInfo &bookInfo() const {
        return book_info_;
    }

    static QString getCoverPath(QWidget *parent, bool validate, QImage *out_image);

    static std::optional<BookInfo>
        getBookInfo(widgetkit::OverlaySurface *surface, const BookInfo &initial);

protected slots:
    void selectCoverImage();

protected:
    void setupUi();
    void setupConnections();

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
