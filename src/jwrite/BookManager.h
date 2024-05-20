#pragma once

#include <QMap>
#include <QUuid>

namespace jwrite {

struct BookInfo {
    QString uuid;
    QString title;
    QString author;
    QString cover_url;
};

class AbstractBookManager {
protected:
    using OptionalStringRef = std::optional<std::reference_wrapper<const QString>>;
    using OptionalString    = std::optional<QString>;

public:
    static QString alloc_uuid();

    virtual ~AbstractBookManager() = default;

    virtual bool uuid_conflicts(const QString &uuid) const {
        return false;
    }

    virtual int alloc_toc_id() const = 0;

    virtual BookInfo &info_ref() = 0;

    virtual const BookInfo &info_ref() const {
        return const_cast<AbstractBookManager *>(this)->info_ref();
    }

    BookInfo info() const {
        return info_ref();
    }

    virtual const QList<int> &get_volumes() const                   = 0;
    virtual const QList<int> &get_chapters_of_volume(int vid) const = 0;
    virtual QList<int>        get_all_chapters() const              = 0;

    virtual int add_volume(int index, const QString &title)           = 0;
    virtual int add_chapter(int vid, int index, const QString &title) = 0;

    virtual int  remove_volume(int vid)  = 0;
    virtual bool remove_chapter(int cid) = 0;

    virtual bool has_volume(int vid) const {
        return get_volumes().contains(vid);
    }

    virtual bool has_chapter(int cid) const {
        return get_all_chapters().contains(cid);
    }

    virtual bool has_toc_item(int id) const {
        return has_volume(id) || has_chapter(id);
    }

    virtual OptionalStringRef get_title(int id) const                    = 0;
    virtual bool              update_title(int id, const QString &title) = 0;

    virtual OptionalString fetch_chapter_content(int cid) = 0;

    virtual bool sync_chapter_content(int cid, const QString &text) = 0;
};

class InMemoryBookManager : public AbstractBookManager {
public:
    InMemoryBookManager()
        : next_toc_id_{0} {}

    virtual ~InMemoryBookManager() = default;

    int alloc_toc_id() const override {
        return next_toc_id_++;
    }

    BookInfo &info_ref() override {
        return info_;
    }

    const QList<int> &get_volumes() const override {
        return vid_list_;
    }

    const QList<int> &get_chapters_of_volume(int vid) const override {
        return const_cast<InMemoryBookManager *>(this)->cid_list_set_[vid];
    }

    QList<int> get_all_chapters() const override;

    int add_volume(int index, const QString &title) override;
    int add_chapter(int vid, int index, const QString &title) override;

    int  remove_volume(int vid) override;
    bool remove_chapter(int cid) override;

    bool has_volume(int vid) const override {
        return cid_list_set_.contains(vid);
    }

    bool has_chapter(int cid) const override {
        return title_pool_.contains(cid) && !cid_list_set_.contains(cid);
    }

    bool has_toc_item(int id) const override {
        return title_pool_.contains(id);
    }

    OptionalStringRef get_title(int id) const override {
        Q_ASSERT(has_toc_item(id));
        if (!has_toc_item(id)) { return std::nullopt; }
        return {const_cast<InMemoryBookManager *>(this)->title_pool_[id]};
    }

    bool update_title(int id, const QString &title) override {
        if (!has_toc_item(id)) { return false; }
        title_pool_[id] = title;
        return true;
    }

private:
    BookInfo              info_;
    QList<int>            vid_list_;
    QMap<int, QList<int>> cid_list_set_;
    QMap<int, QString>    title_pool_;
    mutable int           next_toc_id_;
};

} // namespace jwrite
