#pragma once

#include <QMap>

namespace jwrite {

struct BookDirManager {
    QList<int> get_volumes() const {
        return vid_list;
    }

    QList<int> get_chapters_of_volume(int vid) const {
        return cid_list_set[vid];
    }

    QList<int> get_all_chapters() const {
        QList<int> results;
        for (auto vid : vid_list) { results << get_chapters_of_volume(vid); }
        return results;
    }

    int                   book_id;
    QList<int>            vid_list;
    QMap<int, QList<int>> cid_list_set;
    QMap<int, QString>    title_pool;
};

} // namespace jwrite
