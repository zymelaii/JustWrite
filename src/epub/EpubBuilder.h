#pragma once

#include <QString>
#include <QList>
#include <QTemporaryDir>
#include <functional>

namespace jwrite::epub {

class EpubBuilder {
public:
    //! proto: void(int vol_index, int chap_index, QString &out_chap_title, QString &out_content)
    using FeedCallback = std::function<void(int, int, QString &, QString &)>;

    EpubBuilder(const QString &filename);

    void         build();
    EpubBuilder &feed(FeedCallback request);

    EpubBuilder &with_name(const QString &book_title) {
        book_title_ = book_title;
        return *this;
    }

    EpubBuilder &with_author(const QString &author) {
        author_ = author;
        return *this;
    }

    EpubBuilder &with_volume(const QString &title, int total_chapters) {
        toc_marker_.append(total_chapters);
        vol_toc_.append(title);
        return *this;
    }

protected:
    void write_meta_inf();
    void write_content_opf();
    void write_stylesheet();
    void write_toc_ncx();
    void write_nav_html();
    void write_title_page();

private:
    QString            filename_;
    QString            book_title_;
    QString            author_;
    QTemporaryDir      build_dir_;
    QStringList        vol_toc_;
    QList<QStringList> chap_toc_;
    QList<int>         toc_marker_;
};

}; // namespace jwrite::epub
