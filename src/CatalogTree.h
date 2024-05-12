#pragma once

#include <QWidget>

namespace Ui {
class CatalogTree;
}; // namespace Ui

struct CatalogTreePrivate;

class CatalogTree : public QWidget {
    Q_OBJECT

public:
    explicit CatalogTree(QWidget *parent = nullptr);
    virtual ~CatalogTree();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    int addVolume(int order, const QString &title);
    int addChapter(int vid, const QString &title);

    int totalVolumes() const {
        return vid_list_.size();
    }

    int volumeAt(int index) const {
        return vid_list_.value(index, -1);
    }

    QList<int> chapters(int vid) const {
        if (!cid_list_set_.contains(vid)) { return {}; }
        return cid_list_set_[vid];
    }

signals:
    void itemClicked(int vid, int cid);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::CatalogTree      *ui;
    QList<int>            vid_list_;
    QMap<int, QList<int>> cid_list_set_;
    QList<QString>        title_list_;
    int                   expanded_vid_;
    int                   hover_row_;
};
