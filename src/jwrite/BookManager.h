#pragma once

#include <jwrite/BookInfo.h>
#include <QMap>

namespace jwrite {

struct BookManager {
    virtual ~BookManager() = default;

    const QList<int> &get_volumes() const {
        return vid_list;
    }

    const QList<int> &get_chapters_of_volume(int vid) const {
        return const_cast<BookManager *>(this)->cid_list_set[vid];
    }

    QList<int> get_all_chapters() const {
        QList<int> results;
        for (auto vid : vid_list) { results << get_chapters_of_volume(vid); }
        return results;
    }

    int add_volume(int index, const QString &title) {
        Q_ASSERT(index >= 0 && index <= vid_list.size());
        const int id = title_pool.size();
        title_pool.insert(id, title);
        vid_list.insert(index, id);
        cid_list_set.insert(id, {});
        return id;
    }

    int add_chapter(int vid, int index, const QString &title) {
        Q_ASSERT(cid_list_set.contains(vid));
        auto &cid_list = cid_list_set[vid];
        Q_ASSERT(index >= 0 && index <= cid_list.size());
        const int id = title_pool.size();
        title_pool.insert(id, title);
        cid_list.insert(index, id);
        return id;
    }

    bool has_chapter(int cid) const {
        return title_pool.contains(cid) && !vid_list.contains(cid);
    }

    const QString &get_title(int id) const {
        Q_ASSERT(title_pool.contains(id));
        return const_cast<BookManager *>(this)->title_pool[id];
    }

    void rename_title(int id, const QString &title) {
        Q_ASSERT(title_pool.contains(id));
        title_pool[id] = title;
    }

    virtual QString get_chapter(int cid) = 0;

    virtual void write_chapter(int cid, const QString &text) = 0;

    BookInfo              info;
    int                   book_id;
    QList<int>            vid_list;
    QMap<int, QList<int>> cid_list_set;
    QMap<int, QString>    title_pool;
};

} // namespace jwrite
