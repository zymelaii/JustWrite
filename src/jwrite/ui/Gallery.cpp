#include <jwrite/ui/Gallery.h>
#include <QPainter>
#include <QPalette>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFileDialog>
#include <QInputDialog>

namespace jwrite::Ui {

Gallery::Gallery(QWidget *parent)
    : QWidget(parent)
    , hover_index_{-1}
    , item_size_(150, 200)
    , item_spacing_{20} {
    setupUi();
    setMouseTracking(true);
}

Gallery::~Gallery() {}

void Gallery::addDisplayCaseItem() {
    const auto filter = "图片 (*.bmp *.jpg *.jpeg *.png)";
    auto       path   = QFileDialog::getOpenFileName(this, "选择封面", "", filter);
    if (path.isEmpty()) { return; }

    auto title = QInputDialog::getText(this, "书名", "请输入书名");
    if (title.isEmpty()) { title = QString("未命名书籍-%1").arg(items_.size() + 1); }

    updateDisplayCaseItem(-1, title, path);
}

void Gallery::updateDisplayCaseItem(int index, const QString &title, const QUrl &cover_url) {
    Q_ASSERT(index == -1 || index >= 0 && index < items_.size());

    if (cover_url.isEmpty()) { return; }

    QImage cover(cover_url.toString());
    if (cover.isNull()) { return; }

    cover = cover.scaled(item_size_, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    DisplayCaseItem item{
        .source = cover_url,
        .cover  = QPixmap::fromImage(std::move(cover)),
        .title  = title,
    };

    if (index == -1) {
        items_.append(std::move(item));
        updateGeometry();
    } else {
        items_[index] = std::move(item);
    }

    update();
}

void Gallery::updateColorTheme(const ColorTheme &color_theme) {
    auto pal = palette();
    pal.setColor(QPalette::Base, color_theme.TextBase);
    pal.setColor(QPalette::Window, color_theme.Window);
    pal.setColor(QPalette::WindowText, color_theme.WindowText);
    setPalette(pal);
}

QSize Gallery::minimumSizeHint() const {
    const auto margins = contentsMargins();
    return getItemRect(0, 0).marginsAdded(margins).size();
}

QSize Gallery::sizeHint() const {
    if (auto w = parentWidget()) {
        const auto margins = contentsMargins();
        const int  width   = w->contentsRect().width();
        const int  height  = heightForWidth(width - margins.left() - margins.right());
        return QSize(width, height);
    }
    return minimumSizeHint();
}

int Gallery::heightForWidth(int width) const {
    const auto margins   = contentsMargins();
    const auto item_rect = getItemRect(0, 0);

    const auto min_size = minimumSizeHint();
    if (width < min_size.width()) { return item_rect.height(); }

    const int cols = (width + item_spacing_) / (item_rect.width() + item_spacing_);
    const int rows = (items_.size() + cols) / cols;
    Q_ASSERT(rows > 0);

    const int height = (item_rect.height() + item_spacing_) * rows - item_spacing_;

    return height + margins.top() + margins.bottom();
}

void Gallery::setupUi() {
    auto font = this->font();
    font.setPointSize(12);
    setFont(font);

    setContentsMargins(40, 40, 40, 40);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

int Gallery::getTitleHeight() const {
    const auto fm = fontMetrics();
    return fm.height() + fm.descent();
}

int Gallery::getTitleOffset() const {
    const auto fm = fontMetrics();
    return fm.descent();
}

QRect Gallery::getItemRect(int row, int col) const {
    const auto fm = fontMetrics();

    const int title_offset = getTitleHeight();
    const int title_height = getTitleHeight();

    const int dx = (item_size_.width() + item_spacing_) * col;
    const int dy = (item_size_.height() + title_offset + title_height + item_spacing_) * row;

    auto bb = QRect(contentsRect().topLeft(), item_size_).translated(dx, dy);
    bb.adjust(0, 0, 0, title_offset + title_height);

    return bb;
}

QRect Gallery::getItemTitleRect(const QRect &item_rect) const {
    const auto fm = fontMetrics();

    const int title_offset = getTitleHeight();
    const int title_height = getTitleHeight();

    auto bb = item_rect.adjusted(4, item_size_.height() + title_offset, -4, 0);
    bb.setHeight(title_height);

    return bb;
}

int Gallery::getItemIndex(const QPoint &pos) const {
    const auto rect      = contentsRect();
    const auto item_rect = getItemRect(0, 0);

    const int width = rect.width();
    const int cols  = (width + item_spacing_) / (item_rect.width() + item_spacing_);

    const auto rel_pos = pos - rect.topLeft();
    const int  col     = (rel_pos.x() + item_spacing_) / (item_rect.width() + item_spacing_);

    if (col >= cols) { return -1; }

    const int row = (rel_pos.y() + item_spacing_) / (item_rect.height() + item_spacing_);

    if (const auto bb = getItemRect(row, col); bb.contains(pos)) { return row * cols + col; }

    return -1;
}

void Gallery::drawDisplayCase(QPainter *p, int index, int row, int col) {
    Q_ASSERT(index == -1 || index >= 0 && index < items_.size());

    const bool has_cover = index != -1;
    const bool hover     = hover_index_ == (index == -1 ? items_.size() : index);

    const auto &pal = palette();
    const auto  fm  = fontMetrics();

    auto background = pal.color(QPalette::Window);
    auto foreground = pal.color(QPalette::WindowText);

    const auto bb       = getItemRect(row, col);
    const auto cover_bb = QRect(bb.topLeft(), item_size_);
    const auto title_bb = getItemTitleRect(bb);

    const int cover_radius = 8;

    QPainterPath clip_path{};
    clip_path.addRect(bb);

    p->save();
    p->setClipPath(clip_path);

    if (has_cover) {
        const auto &item = items_[index];

        p->drawPixmap(cover_bb, item.cover);

        if (hover) {
            background.setAlpha(100);
            p->setPen(Qt::transparent);
            p->setBrush(background);
            p->drawRoundedRect(cover_bb, cover_radius, cover_radius);
        }

        const auto title = fm.elidedText(item.title, Qt::ElideRight, title_bb.width());
        p->setPen(pal.color(QPalette::WindowText));
        p->drawText(title_bb, Qt::AlignCenter, title);
    } else {
        if (!hover) {
            background.setAlpha(200);
            foreground.setAlpha(200);
        }

        p->setPen(Qt::transparent);
        p->setBrush(background);
        p->drawRoundedRect(cover_bb, cover_radius, cover_radius);

        const int width  = cover_bb.width() / 2.5;
        const int height = 10;
        const int x0     = cover_bb.x() + (cover_bb.width() - width) / 2;
        const int x1     = x0 + (width - height) / 2;
        const int x2     = x1 + height;
        const int x3     = x2 + (width - height) / 2;
        const int y0     = cover_bb.y() + (cover_bb.height() - width) / 2;
        const int y1     = y0 + (width - height) / 2;
        const int y2     = y1 + height;
        const int y3     = y2 + (width - height) / 2;

        QPolygon icon_add;
        icon_add << QPoint(x0, y1) << QPoint(x0, y2) << QPoint(x1, y2) << QPoint(x1, y3)
                 << QPoint(x2, y3) << QPoint(x2, y2) << QPoint(x3, y2) << QPoint(x3, y1)
                 << QPoint(x2, y1) << QPoint(x2, y1) << QPoint(x2, y0) << QPoint(x1, y0)
                 << QPoint(x1, y1);

        QPainterPath path{};
        path.addPolygon(icon_add);

        p->fillPath(path, foreground);
    }

    p->restore();
}

void Gallery::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    const int width = contentsRect().width();
    const int cols  = (width + item_spacing_) / (item_size_.width() + item_spacing_);

    for (int i = 0; i < items_.size(); ++i) { drawDisplayCase(&p, i, i / cols, i % cols); }

    drawDisplayCase(&p, -1, items_.size() / cols, items_.size() % cols);
}

void Gallery::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);

    hover_index_ = -1;
    update();
}

void Gallery::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);

    hover_index_ = getItemIndex(event->pos());
    update();
}

void Gallery::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);

    if (hover_index_ == -1) { return; }

    if (hover_index_ != items_.size()) { return; }

    emit clicked(hover_index_);

    // addDisplayCaseItem();
}

void Gallery::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

} // namespace jwrite::Ui