#include <jwrite/ui/PlainImageView.h>
#include <QPainter>

namespace jwrite::ui {

PlainImageView::PlainImageView(QWidget *parent)
    : QWidget(parent)
    , border_visible_{true} {
    setupUi();
}

PlainImageView::~PlainImageView() {}

void PlainImageView::setImage(const QString &path) {
    image_path_ = path;
    reloadImage();
}

void PlainImageView::setViewSize(const QSize &size) {
    view_size_ = size;
    updateGeometry();
    reloadImage();
}

void PlainImageView::setBorderVisible(bool visible) {
    border_visible_ = visible;
    update();
}

QSize PlainImageView::minimumSizeHint() const {
    return QRect(QPoint(0, 0), view_size_).marginsAdded(contentsMargins()).size();
}

QSize PlainImageView::sizeHint() const {
    return minimumSizeHint();
}

void PlainImageView::setupUi() {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void PlainImageView::reloadImage() {
    QPixmap pixmap(image_path_);
    if (pixmap.isNull()) { return; }
    cached_pixmap_ =
        pixmap.scaled(view_size_, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    update();
}

void PlainImageView::paintEvent(QPaintEvent *event) {
    if (view_size_.isEmpty()) { return; }

    QPainter    p(this);
    const auto &pal = palette();

    p.setRenderHint(QPainter::Antialiasing);

    const auto bb = contentsRect();

    if (!cached_pixmap_.isNull()) { p.drawPixmap(bb, cached_pixmap_); }

    if (border_visible_) {
        p.setPen(QPen(pal.color(QPalette::Window), 0.5));
        p.drawRect(bb.adjusted(1, 1, -1, -1));
    }
}

} // namespace jwrite::ui
