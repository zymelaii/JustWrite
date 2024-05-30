#include <jwrite/ui/FloatingLabel.h>
#include <jwrite/AppConfig.h>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QMouseEvent>

using namespace widgetkit;

namespace jwrite::ui {

void FloatingLabel::set_text(const QString &text) {
    setText(text);
    update_geometry();
}

void FloatingLabel::update_geometry() {
    if (auto w = parentWidget()) {
        const auto size      = sizeHint();
        const int  spacing_x = 32;
        const int  spacing_y = 16;
        const auto rect      = w->contentsRect();
        const int  pos_x     = rect.right() - size.width() - spacing_x;
        const int  pos_y     = rect.top() + spacing_y;
        setGeometry(QRect(QPoint(pos_x, pos_y), size));
    } else {
        setGeometry(0, 0, 0, 0);
    }
}

FloatingLabel::FloatingLabel(QWidget *parent)
    : ClickableLabel(parent) {
    Q_ASSERT(parent);

    setupUi();

    parent->installEventFilter(this);
    update_geometry();
}

FloatingLabel::~FloatingLabel() {}

QSize FloatingLabel::minimumSizeHint() const {
    const auto margins = contentsMargins();
    const auto margin_size =
        QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    const auto fm        = fontMetrics();
    const auto text_size = fm.size(Qt::TextSingleLine, text());
    const int  padding_x = 10;
    const int  padding_y = 4;
    const auto size = QSize(text_size.width() + padding_x * 2, text_size.height() + padding_y * 2);
    return size + margin_size;
}

QSize FloatingLabel::sizeHint() const {
    return minimumSizeHint();
}

void FloatingLabel::setupUi() {
    auto font = this->font();
    font.setPointSize(8);
    setFont(font);

    setContentsMargins(4, 4, 4, 4);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

bool FloatingLabel::eventFilter(QObject *watched, QEvent *event) {
    switch (event->type()) {
        case QEvent::Move:
            [[fallthrough]];
        case QEvent::Resize: {
            update_geometry();
        } break;
        case QEvent::MouseButtonPress:
            [[fallthrough]];
        case QEvent::MouseButtonRelease:
            [[fallthrough]];
        case QEvent::MouseButtonDblClick:
            [[fallthrough]];
        case QEvent::MouseMove: {
            const auto e   = static_cast<QMouseEvent *>(event);
            const auto pos = mapFromGlobal(e->globalPosition()).toPoint();
            if (contentsRect().contains(pos)) { return true; }
        } break;
        default: {
        } break;
    }
    return QWidget::eventFilter(watched, event);
}

bool FloatingLabel::event(QEvent *event) {
    if (!parent()) { return QWidget::event(event); }
    switch (event->type()) {
        case QEvent::ParentChange: {
            parent()->installEventFilter(this);
            update_geometry();
            break;
        }
        case QEvent::ParentAboutToChange: {
            parent()->removeEventFilter(this);
            break;
        }
        default:
            break;
    }
    return QWidget::event(event);
}

void FloatingLabel::paintEvent(QPaintEvent *event) {
    if (size().isEmpty() || text().isEmpty()) { return; }

    QPainter    p(this);
    const auto &pal = palette();

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const auto bb     = contentsRect();
    const int  radius = 4;

    const auto &background = pal.window();
    const auto &foreground = pal.windowText();
    const auto &border     = pal.base();

    p.setBrush(background);
    p.setPen(border.color());
    p.drawRoundedRect(bb, radius, radius);

    p.setPen(foreground.color());
    p.drawText(bb, Qt::AlignCenter, text());
}

} // namespace jwrite::ui
