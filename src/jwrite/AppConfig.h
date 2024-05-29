#pragma once

#include <jwrite/ColorScheme.h>
#include <QObject>

namespace jwrite {

class AppConfig : public QObject {
    Q_OBJECT

public:
    enum class StandardPath {
        AppHome,
        UserData,
    };

public:
    AppConfig();
    ~AppConfig() override;

signals:
    void on_theme_change(ColorTheme theme);
    void on_scheme_change(const ColorScheme& scheme);

public:
    QString theme_name() const;

    ColorTheme theme() const {
        return theme_;
    }

    void set_theme(ColorTheme theme);

    const ColorScheme& scheme() const {
        return scheme_;
    }

    void set_scheme(const ColorScheme& scheme);

    QString icon(const QString& name) const;

    QString path(StandardPath path_type) const;

    QString settings_file() const;

    static AppConfig& get_instance();

    void load();
    void save();

private:
    ColorTheme  theme_;
    ColorScheme scheme_;
};

} // namespace jwrite
