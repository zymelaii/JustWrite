#include "StatusBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

namespace jwrite {

struct StatusBarPrivate {
    QHBoxLayout *layout;
    int          left_aligned_items;

    StatusBarPrivate() {
        layout             = nullptr;
        left_aligned_items = 0;
    }
};

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent)
    , d{new StatusBarPrivate} {
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins({});
    d->layout->addStretch();

    setAutoFillBackground(true);
}

StatusBar::~StatusBar() {
    delete d;
}

QSize StatusBar::minimumSizeHint() const {
    const auto margins = contentsMargins();
    const int  height  = fontMetrics().height();
    return QSize(0, height + margins.top() + margins.bottom());
}

QSize StatusBar::sizeHint() const {
    return minimumSizeHint();
}

void StatusBar::setSpacing(int spacing) {
    d->layout->setSpacing(spacing);
}

QLabel *StatusBar::addItem(const QString &text) {
    const int index = d->left_aligned_items++;
    auto      label = new QLabel(text);
    d->layout->insertWidget(index, label);
    return label;
}

QLabel *StatusBar::addItemAtRightSide(const QString &text) {
    const int index = d->layout->count() - d->left_aligned_items + 1;
    auto      label = new QLabel(text);
    d->layout->insertWidget(index, label);
    return label;
}

} // namespace jwrite
