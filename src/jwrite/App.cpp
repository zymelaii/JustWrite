#include <jwrite/ui/JustWrite.h>
#include <jwrite/ProfileUtils.h>
#include <jwrite/Version.h>
#include <QGuiApplication>
#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <memory>

constexpr QSize PREFERRED_CLIENT_SIZE(1000, 600);

QScreen *get_current_screen() {
    return QGuiApplication::screenAt(QCursor::pos());
}

QRect compute_preferred_geometry(const QRect &parent_geo) {
    const auto w    = qMin(parent_geo.width(), PREFERRED_CLIENT_SIZE.width());
    const auto h    = qMin(parent_geo.height(), PREFERRED_CLIENT_SIZE.height());
    const auto left = parent_geo.left() + (parent_geo.width() - w) / 2;
    const auto top  = parent_geo.top() + (parent_geo.height() - h) / 2;
    return QRect(left, top, w, h);
}

int main(int argc, char *argv[]) {
    //! TODO: ensure single jwrite instance in the system
    //! HINT: or ensure the book is always editable in only one instance

    QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QApplication app(argc, argv);

    QApplication::setOrganizationDomain("jwrite");
    QApplication::setOrganizationName("github.com/zymelaii/jwrite");
    QApplication::setApplicationName("JustWrite");
    QApplication::setApplicationDisplayName("只写");
    QApplication::setApplicationVersion(jwrite::VERSION.toString());

    const auto font_name = u8"更纱黑体 SC Light";
    QFontDatabase::addApplicationFont(QString("fonts/%1.ttf").arg(font_name));
    QApplication::setFont(QFont(font_name, 16));

    auto       screen     = get_current_screen();
    const auto screen_geo = screen->geometry();

    setup_jwrite_profiler(10);

    auto client = std::make_unique<jwrite::ui::JustWrite>();
    client->setGeometry(compute_preferred_geometry(screen_geo));
    client->show();

    return app.exec();
}
