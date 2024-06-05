#include <jwrite/ui/JustWrite.h>
#include <jwrite/ProfileUtils.h>
#include <jwrite/Version.h>
#include <jwrite/AppConfig.h>
#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <QMouseEvent>
#include <memory>
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using jwrite::AppConfig;
using jwrite::ui::JustWrite;

class JwriteApplication : public QApplication {
public:
    JwriteApplication(int &argc, char **argv)
        : QApplication(argc, argv)
        , client_{nullptr} {}

public:
    void bind(JustWrite *client) {
        client_ = client;
    }

protected:
    void notify_client_on_mouse_move(QWidget *receiver, QMouseEvent *event) {
        Q_ASSERT(client_);
        if (receiver == client_) { return; }

        //! ATTENTION: this only ensures that jwrite can listen for mouse-move-events from the child
        //! widgets, YOU must enables the mouse tracking for jwrite to ensure that it receives these
        //! events properly

        auto cloned = new QMouseEvent(
            QEvent::MouseMove,
            client_->mapFromGlobal(event->globalPosition()),
            event->globalPosition(),
            event->button(),
            event->buttons(),
            event->modifiers());
        postEvent(const_cast<JustWrite *>(client_), cloned);
    }

    bool filter_key_press_event(QKeyEvent *event) {
        Q_ASSERT(client_);
        if (auto opt = client_->try_match_shortcut(event)) {
            client_->trigger_shortcut(*opt);
            return true;
        }
        return false;
    }

    bool notify(QObject *receiver, QEvent *event) override {
        if (client_ && receiver->isWidgetType()) {
            const auto w = qobject_cast<QWidget *>(receiver);
            switch (event->type()) {
                case QEvent::MouseMove: {
                    notify_client_on_mouse_move(w, static_cast<QMouseEvent *>(event));
                } break;
                case QEvent::KeyPress: {
                    if (const bool done = filter_key_press_event(static_cast<QKeyEvent *>(event))) {
                        return true;
                    }
                } break;
                case QEvent::ScreenChangeInternal: {
                    if (client_ == w && client_->is_fullscreen_mode()) {
                        //! NOTE: switch screen in fullscreen mode will destroy the original
                        //! geometry, force to refresh it to keep the fullscreenn mode behaves well
                        client_->set_fullscreen_mode(false);
                        client_->set_fullscreen_mode(true);
                    }
                }
                default: {
                } break;
            }
        }
        return QApplication::notify(receiver, event);
    }

private:
    JustWrite *client_;
};

constexpr QSize PREFERRED_CLIENT_SIZE(1000, 600);

QScreen *get_current_screen() {
    return QApplication::screenAt(QCursor::pos());
}

QRect compute_preferred_geometry(const QRect &parent_geo) {
    const auto w    = qMin(parent_geo.width(), PREFERRED_CLIENT_SIZE.width());
    const auto h    = qMin(parent_geo.height(), PREFERRED_CLIENT_SIZE.height());
    const auto left = parent_geo.left() + (parent_geo.width() - w) / 2;
    const auto top  = parent_geo.top() + (parent_geo.height() - h) / 2;
    return QRect(left, top, w, h);
}

void load_default_fonts() {
    const auto &config        = AppConfig::get_instance();
    const auto  home          = config.path(AppConfig::StandardPath::AppHome);
    const auto  font_path_fmt = QString("%1/fonts/SarasaGothicSC-%2.ttf").arg(home);

    for (const auto font_name : magic_enum::enum_values<AppConfig::FontStyle>()) {
        const auto &path = font_path_fmt.arg(magic_enum::enum_name(font_name).data());
        if (!QFile::exists(path)) { continue; }
        const int id = QFontDatabase::addApplicationFont(path);
        Q_ASSERT(id != -1);
    }

    QApplication::setFont(config.font(AppConfig::FontStyle::Light, 16));
}

void init_logger() {
    auto      &config   = AppConfig::get_instance();
    const auto log_path = QString("%1/jwrite-%2.log")
                              .arg(config.path(AppConfig::StandardPath::Log))
                              .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmm"));
#ifdef NDEBUG
    auto logger = spdlog::basic_logger_mt("jwrite.default", log_path.toLocal8Bit().toStdString());
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] %v");
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);
#else
    auto con_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    con_sink->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%l]%$ %v");
    con_sink->set_level(spdlog::level::trace);

    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.toLocal8Bit().toStdString());
    file_sink->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] %v");
    file_sink->set_level(spdlog::level::info);

    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "jwrite.default", spdlog::sinks_init_list{con_sink, file_sink}));
#endif
}

void init_language() {
    auto       translator = new QTranslator;
    const bool succeed    = translator->load(":/lang.zh_CN");
    Q_ASSERT(succeed);
    QApplication::installTranslator(translator);
}

int main(int argc, char *argv[]) {
    //! TODO: ensure single jwrite instance in the system
    //! HINT: or ensure the book is always editable in only one instance

    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    JwriteApplication app(argc, argv);

    init_logger();
    init_language();

    spdlog::info("start up jwrite v{}", jwrite::VERSION.toString().toStdString());

    QApplication::setOrganizationDomain("jwrite");
    QApplication::setOrganizationName("github.com/zymelaii/jwrite");
    QApplication::setApplicationName("JustWrite");
    QApplication::setApplicationDisplayName(QObject::tr("App.display_name"));
    QApplication::setApplicationVersion(jwrite::VERSION.toString());

    load_default_fonts();

    auto       screen     = get_current_screen();
    const auto screen_geo = screen->geometry();

    setup_jwrite_profiler(10);

    auto client = std::make_unique<JustWrite>();
    app.bind(client.get());

    client->setGeometry(compute_preferred_geometry(screen_geo));
    client->show();

    spdlog::info("Hello, JustWrite!");
    const int exit_code = app.exec();
    spdlog::info("jwrite exited with code {}", exit_code);
    spdlog::info("Bye, JustWrite!");

    return exit_code;
}
