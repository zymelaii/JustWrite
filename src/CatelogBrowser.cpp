#include "CatelogBrowser.h"
#include <QItemDelegate>
#include <QStandardItemModel>
#include <QScrollBar>

struct CatelogBrowserPrivate {
    QStandardItemModel *model;

    CatelogBrowserPrivate() {
        model = new QStandardItemModel;
    }
};

CatelogBrowser::CatelogBrowser(QWidget *parent)
    : QTreeView(parent)
    , d{new CatelogBrowserPrivate} {
    setFrameStyle(QFrame::NoFrame);
    setHeaderHidden(true);

    //! TODO: remove
    auto root      = d->model->invisibleRootItem();
    auto vol_item  = new QStandardItem(QIcon(":/res/icons/light/folder-2-line.png"), "第一卷");
    auto chap_item = new QStandardItem(QIcon(":/res/icons/light/article-line.png"), "第一章");
    vol_item->appendRow(chap_item);
    root->appendRow(vol_item);

    setModel(d->model);

    verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0px 0px 3px 0px;
        }
        QScrollBar::add-page, QScrollBar::sub-page {
            background: transparent;
        }
        QScrollBar::handle:vertical {
            background: #373737;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #404040;
        }
        QScrollBar::add-line:vertical {
            height: 0px;
        }
        QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    setStyleSheet(R"(
        QTreeView {
            background-color: transparent;
            outline: none;
            color: white;
        }
        QTreeView::branch:open:has-children:!has-siblings, QTreeView::branch:open:has-children:has-siblings {
            image: url(:/res/icons/light/arrow-drop-down-line.png);
        }
        QTreeView::branch:has-children:!has-siblings:closed, QTreeView::branch:closed:has-children:has-siblings {
            image: url(:/res/icons/light/arrow-drop-right-line.png);
        }
        QTreeView::item:hover {
            background-color: transparent;
        }
        QTreeView::item:selected {
            background-color: #2a2e2d;
            border: none;
        }
    )");
}

CatelogBrowser::~CatelogBrowser() {
    delete d;
}
