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

signals:
    void requestNewVolume();
    void requestNewChapter();

private:
    Ui::JustWriteSidebar *ui;
};
