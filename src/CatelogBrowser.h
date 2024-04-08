#pragma once

#include <QTreeView>

struct CatelogBrowserPrivate;

class CatelogBrowser : public QTreeView {
    Q_OBJECT

public:
    explicit CatelogBrowser(QWidget *parent = nullptr);
    virtual ~CatelogBrowser();

private:
    CatelogBrowserPrivate *d;
};
