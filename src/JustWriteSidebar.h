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
    void newVolume(int index, const QString &title);
    void newChapter(int volume_index, const QString &title);
    void openEmptyChapter();

signals:
    void chapterOpened(int cid);

private:
    Ui::JustWriteSidebar *ui;
};
