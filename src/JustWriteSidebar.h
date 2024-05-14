#pragma once

#include <QWidget>

namespace Ui {
struct JustWriteSidebar;
}; // namespace Ui

class JustWriteSidebar : public QWidget {
    Q_OBJECT

public:
    explicit JustWriteSidebar(QWidget *parent = nullptr);
    virtual ~JustWriteSidebar();

public slots:
    void create_new_volume(int index, const QString &title);
    void create_new_chapter(int volume_index, const QString &title);
    void open_empty_chapter();

signals:
    void chapterOpened(int cid);

private:
    Ui::JustWriteSidebar *ui;
};
