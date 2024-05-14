#include "CatalogTree.h"
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>

namespace Ui {
class CatalogTree {
public:
    void setup(QWidget *self) {
        auto font = self->font();
        font.setPointSize(10);
        self->setFont(font);

        self->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    }
};
}; // namespace Ui

CatalogTree::CatalogTree(QWidget *parent)
    : QWidget(parent)
    , expanded_vid_{-1}
    , hover_row_{-1}
    , ui{new Ui::CatalogTree} {
    ui->setup(this);
    setMouseTracking(true);
}

CatalogTree::~CatalogTree() {
    delete ui;
}

QSize CatalogTree::sizeHint() const {
    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    const auto height     = fm.descent() + row_height * title_list_.size();
    QRect      rect(0, 0, fm.averageCharWidth() * 4, height);
    return rect.marginsAdded(contentsMargins()).size();
}

QSize CatalogTree::minimumSizeHint() const {
    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    QRect      rect(0, 0, fm.averageCharWidth() * 4, fm.descent() + row_height);
    return rect.marginsAdded(contentsMargins()).size();
}

int CatalogTree::add_volume(int order, const QString &title) {
    if (order < -1 || order > static_cast<int>(vid_list_.size())) { return -1; }
    if (order == -1) { order = vid_list_.size(); }
    const auto vid = static_cast<int>(title_list_.size());
    title_list_.push_back(title);
    vid_list_.insert(order, vid);
    cid_list_set_.insert(vid, {});
    update();
    return vid;
}

int CatalogTree::add_chapter(int vid, const QString &title) {
    if (!cid_list_set_.contains(vid)) { return -1; }
    auto      &chaps = cid_list_set_[vid];
    const auto cid   = static_cast<int>(title_list_.size());
    chaps.push_back(cid);
    title_list_.push_back(title);
    update();
    return cid;
}

void CatalogTree::paintEvent(QPaintEvent *event) {
    QPainter   p(this);
    const auto pal = palette();

    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    const auto flags      = Qt::AlignLeft | Qt::AlignVCenter;

    p.setPen(pal.color(QPalette::Text));
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    auto rect = contentsRect();
    rect.translate(row_height, fm.descent());
    rect.setHeight(row_height);
    const auto row_rc          = QRect(row_height, 0, rect.width() - row_height, row_height);
    const auto highlight_color = QColor(255, 255, 255, 30);

    int index   = 0;
    int v_index = 0;
    int c_index = 0;
    for (auto vid : vid_list_) {
        const auto title = QStringLiteral("第 %1 卷 %2").arg(++v_index).arg(title_list_[vid]);
        p.drawText(rect, flags, title);
        ++index;
        rect.translate(16, row_height);
        for (auto cid : cid_list_set_[vid]) {
            const auto title = QStringLiteral("第 %1 章 %2").arg(++c_index).arg(title_list_[cid]);
            p.drawText(rect, flags, title);
            if (index++ == hover_row_) {
                p.fillRect(row_rc.translated(0, rect.top()), highlight_color);
            }
            rect.translate(0, row_height);
        }
        rect.translate(-16, 0);
    }
}

void CatalogTree::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);

    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    auto       rect       = contentsRect();
    rect.adjust(row_height, fm.descent(), 0, 0);

    const int last_hover_row = hover_row_;

    if (rect.contains(e->pos())) {
        hover_row_ = (e->pos().y() - rect.top()) / row_height;
    } else {
        hover_row_ = -1;
    }

    if (hover_row_ != last_hover_row) { update(); }
}

void CatalogTree::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);

    if (hover_row_ != -1) {
        hover_row_ = -1;
        update();
    }
}

void CatalogTree::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    if (e->button() == Qt::MiddleButton) { return; }
    if (hover_row_ == -1) { return; }
    if (hover_row_ >= static_cast<int>(title_list_.size())) { return; }

    int index        = 0;
    int vid_selected = -1;
    int cid_selected = -1;
    for (auto vid : vid_list_) {
        if (index++ == hover_row_) { break; }
        for (auto cid : cid_list_set_[vid]) {
            if (index++ == hover_row_) {
                vid_selected = vid;
                cid_selected = cid;
                break;
            }
        }
    }

    if (vid_selected == -1 || cid_selected == -1) { return; }

    if (e->button() == Qt::LeftButton) {
        emit itemClicked(vid_selected, cid_selected);
    } else if (e->button() == Qt::RightButton) {
        //! TODO: open context menu
    }
}
