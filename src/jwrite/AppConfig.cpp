#include <jwrite/AppConfig.h>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <memory>

namespace jwrite {

AppConfig::AppConfig()
    : QObject() {
    load();
}

AppConfig::~AppConfig() {}

QString AppConfig::theme_name() const {
    switch (theme_) {
        case ColorTheme::Light: {
            return "light";
        } break;
        case ColorTheme::Dark: {
            return "dark";
        } break;
    }
}

void AppConfig::set_theme(ColorTheme theme) {
    theme_ = theme;
    emit on_theme_change(theme_);
}

void AppConfig::set_scheme(const ColorScheme& scheme) {
    scheme_ = scheme;
    emit on_scheme_change(scheme_);
}

QString AppConfig::icon(QString name) const {
    return QString(":/res/themes/%1/icons/%2").arg(theme_name()).arg(name);
}

QString AppConfig::path(StandardPath path_type) const {
    switch (path_type) {
        case StandardPath::AppHome: {
            return QDir::cleanPath(QCoreApplication::applicationDirPath());
        } break;
        case StandardPath::UserData: {
            return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/data");
        } break;
    }
}

QString AppConfig::settings_file() const {
    return QDir::cleanPath(path(StandardPath::UserData) + "/settings.ini");
}

AppConfig& AppConfig::get_instance() {
    static std::unique_ptr<AppConfig> instance{nullptr};
    if (!instance) { instance = std::make_unique<AppConfig>(); }
    return *instance;
}

void AppConfig::load() {
    const auto settings_file = this->settings_file();

    if (!QFile::exists(settings_file)) {
        theme_  = ColorTheme::Dark;
        scheme_ = ColorScheme::get_default_scheme_of_theme(theme_);
        return;
    }

    const QList<QPair<QString, ColorScheme::ColorRole>> color_roles = {
        {"window",        ColorScheme::Window      },
        {"window_text",   ColorScheme::WindowText  },
        {"border",        ColorScheme::Border      },
        {"panel",         ColorScheme::Panel       },
        {"text",          ColorScheme::Text        },
        {"text_base",     ColorScheme::TextBase    },
        {"selected_text", ColorScheme::SelectedText},
        {"hover",         ColorScheme::Hover       },
        {"selected_item", ColorScheme::SelectedItem},
    };

    QSettings settings(settings_file, QSettings::IniFormat);

    settings.beginGroup("style");
    const auto theme_name = settings.value("theme", "dark").toString();
    settings.endGroup();

    theme_ = theme_name.toLower() == "light" ? ColorTheme::Light : ColorTheme::Dark;

    auto scheme = ColorScheme::get_default_scheme_of_theme(theme_);
    settings.beginGroup("scheme");
    for (const auto& [name, role] : color_roles) {
        if (const auto color_name = settings.value(name).toString(); !color_name.isEmpty()) {
            if (auto color = QColor::fromString(color_name); color.isValid()) {
                scheme[role] = color;
            }
        }
    }
    settings.endGroup();

    scheme_ = scheme;
}

void AppConfig::save() {
    const QList<QPair<QString, ColorScheme::ColorRole>> color_roles = {
        {"window",        ColorScheme::Window      },
        {"window_text",   ColorScheme::WindowText  },
        {"border",        ColorScheme::Border      },
        {"panel",         ColorScheme::Panel       },
        {"text",          ColorScheme::Text        },
        {"text_base",     ColorScheme::TextBase    },
        {"selected_text", ColorScheme::SelectedText},
        {"hover",         ColorScheme::Hover       },
        {"selected_item", ColorScheme::SelectedItem},
    };

    QSettings settings(settings_file(), QSettings::IniFormat);
    settings.beginGroup("style");
    const auto theme_name = settings.value("theme", "dark").toString();
    settings.setValue("theme", QString{magic_enum::enum_name(theme_).data()}.toLower());
    settings.endGroup();
    settings.beginGroup("scheme");
    for (const auto& [name, role] : color_roles) { settings.setValue(name, scheme_[role].name()); }
    settings.endGroup();
}

} // namespace jwrite
