#include <jwrite/AppConfig.h>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <memory>
#include <QFontDatabase>
#include <array>
#include <ctype.h>
#include <stddef.h>
#include <magic_enum.hpp>
#include <toml++/toml.h>
#include <spdlog/spdlog.h>

namespace jwrite {

static std::string convert_pascal_case_to_snake_case(const std::string_view pascal_name) {
    std::string sneak_name{};
    for (int i = 0; i < pascal_name.size(); i++) {
        const auto c = pascal_name[i];
        if (isupper(c)) {
            if (i > 0) { sneak_name.push_back('_'); }
            sneak_name.push_back(tolower(c));
        } else {
            sneak_name.push_back(c);
        }
    }
    return sneak_name;
}

#define AUTO_NAME_WRAPPER(macro, root, option) \
    macro(root, option, convert_pascal_case_to_snake_case(#option))

#define OPTIONAL_OPTION_AUTO_NAME_WRAPPER(macro, root, option, val_option, val_option_name) \
    macro(root, option, convert_pascal_case_to_snake_case(#option), val_option, val_option_name)

#define PARSE_VALUE_OPTION(root, option, type, method, name)       \
    if (auto opt = root->get_as<type>(name)) {                     \
        set_value(ValOption::option, QString::method(opt->get())); \
    }

#define PARSE_BOOL_OPTION_BASE(root, option, name) \
    if (auto opt = root->get_as<bool>(name)) { set_option(Option::option, opt->get()); }
#define PARSE_BOOL_OPTION(root, option)               AUTO_NAME_WRAPPER(PARSE_BOOL_OPTION_BASE, root, option)
#define PARSE_BOOL_OPTION_MANNUAL(root, option, name) PARSE_BOOL_OPTION_BASE(root, option, #name)

#define PARSE_STR_OPTION_BASE(root, option, name) \
    PARSE_VALUE_OPTION(root, option, std::string, fromStdString, name)
#define PARSE_STR_OPTION(root, option)               AUTO_NAME_WRAPPER(PARSE_STR_OPTION_BASE, root, option)
#define PARSE_STR_OPTION_MANNUAL(root, option, name) PARSE_STR_OPTION_BASE(root, option, #name)

#define PARSE_INT_OPTION_BASE(root, option, name) \
    PARSE_VALUE_OPTION(root, option, int64_t, number, name)
#define PARSE_INT_OPTION(root, option)               AUTO_NAME_WRAPPER(PARSE_INT_OPTION_BASE, root, option)
#define PARSE_INT_OPTION_MANNUAL(root, option, name) PARSE_INT_OPTION_BASE(root, option, #name)

#define PARSE_DOUBLE_OPTION_BASE(root, option, name) \
    PARSE_VALUE_OPTION(root, option, double, number, name)
#define PARSE_DOUBLE_OPTION(root, option) AUTO_NAME_WRAPPER(PARSE_DOUBLE_OPTION_BASE, root, option)
#define PARSE_DOUBLE_OPTION_MANNUAL(root, option, name) \
    PARSE_DOUBLE_OPTION_BASE(root, option, #name)

#define PARSE_STR_ARRAY_OPTION_BASE(root, option, name)   \
    if (auto opt = root->get_as<toml::array>(name)) {     \
        QStringList parts{};                              \
        for (auto& val : *opt) {                          \
            const auto part = val.as_string();            \
            if (!part) { continue; }                      \
            parts << QString::fromStdString(part->get()); \
        }                                                 \
        set_value(ValOption::option, parts.join(','));    \
    }
#define PARSE_STR_ARRAY_OPTION(root, option) \
    AUTO_NAME_WRAPPER(PARSE_STR_ARRAY_OPTION_BASE, root, option)
#define PARSE_STR_ARRAY_OPTION_MANNUAL(root, option, name) \
    PARSE_STR_ARRAY_OPTION_BASE(root, option, #name)

#define PARSE_OPTIONAL_INT_OPTION_BASE(root, option, option_name, val_option, val_option_name) \
    if (auto opt = root->get_as<toml::table>(option_name)) {                                   \
        if (auto enabled = opt->get_as<bool>("enabled")) {                                     \
            set_option(Option::option, enabled->get());                                        \
        }                                                                                      \
        if (auto val = opt->get_as<int64_t>(val_option_name)) {                                \
            set_value(ValOption::val_option, QString::number(val->get()));                     \
        }                                                                                      \
    }
#define PARSE_OPTIONAL_INT_OPTION(root, option, val_option, val_option_name) \
    OPTIONAL_OPTION_AUTO_NAME_WRAPPER(                                       \
        PARSE_OPTIONAL_INT_OPTION_BASE, root, option, val_option, #val_option_name)
#define PARSE_OPTIONAL_INT_OPTION_MANNUAL(root, option, option_name, val_option, val_option_name) \
    PARSE_OPTIONAL_INT_OPTION_BASE(root, option, #option_name, val_option, #val_option_name)

#define PARSE_OPTIONAL_DOUBLE_OPTION_BASE(root, option, option_name, val_option, val_option_name) \
    if (auto opt = root->get_as<toml::table>(option_name)) {                                      \
        if (auto enabled = opt->get_as<bool>("enabled")) {                                        \
            set_option(Option::option, enabled->get());                                           \
        }                                                                                         \
        if (auto val = opt->get_as<double>(val_option_name)) {                                    \
            set_value(ValOption::val_option, QString::number(val->get()));                        \
        }                                                                                         \
    }
#define PARSE_OPTIONAL_DOUBLE_OPTION(root, option, val_option, val_option_name) \
    OPTIONAL_OPTION_AUTO_NAME_WRAPPER(                                          \
        PARSE_OPTIONAL_DOUBLE_OPTION_BASE, root, option, val_option, #val_option_name)
#define PARSE_OPTIONAL_DOUBLE_OPTION_MANNUAL(               \
    root, option, option_name, val_option, val_option_name) \
    PARSE_OPTIONAL_DOUBLE_OPTION_BASE(root, option, #option_name, val_option, #val_option_name)

class BlockSignalGuard {
public:
    BlockSignalGuard(QObject* object)
        : object_{object}
        , signal_blocked_{object->signalsBlocked()} {
        this->object_->blockSignals(true);
    }

    ~BlockSignalGuard() {
        object_->blockSignals(signal_blocked_);
    }

private:
    QObject*   object_;
    const bool signal_blocked_;
};

template <typename E>
class FullCoverageEnumVisitor {
public:
    ~FullCoverageEnumVisitor() {
#ifndef NDEBUG
        for (const auto& value : magic_enum::enum_values<E>()) {
            if (!visited_.contains(value)) {
                qDebug().noquote() << "enum value not visited:" << magic_enum::enum_name(value);
            }
        }
#endif
        Q_ASSERT(magic_enum::enum_count<E>() == visited_.size());
    }

    E operator[](E value) {
        visited_.insert(value);
        return value;
    }

private:
    QSet<E> visited_;
};

static auto get_color_roles_for_settings() {
    const QList<QPair<QString, ColorScheme::ColorRole>> color_roles = {
        {"window",               ColorScheme::Window            },
        {"window_text",          ColorScheme::WindowText        },
        {"border",               ColorScheme::Border            },
        {"panel",                ColorScheme::Panel             },
        {"text",                 ColorScheme::Text              },
        {"text_base",            ColorScheme::TextBase          },
        {"selected_text",        ColorScheme::SelectedText      },
        {"hover",                ColorScheme::Hover             },
        {"selected_item",        ColorScheme::SelectedItem      },
        {"floating_item",        ColorScheme::FloatingItem      },
        {"floating_item_border", ColorScheme::FloatingItemBorder},
    };
    Q_ASSERT(color_roles.size() == magic_enum::enum_count<ColorScheme::ColorRole>());
    return color_roles;
}

AppConfig::AppConfig()
    : QObject() {}

AppConfig::~AppConfig() {}

QString AppConfig::theme_name() const {
    return QString::fromLatin1(magic_enum::enum_name(theme_).data()).toLower();
}

void AppConfig::set_theme(ColorTheme theme) {
    theme_ = theme;
    emit on_theme_change(theme_);
}

void AppConfig::set_scheme(const ColorScheme& scheme) {
    scheme_ = scheme;
    emit on_scheme_change(scheme_);
}

QString AppConfig::icon(const QString& name) const {
    return QString(":/res/themes/%1/icons/%2.svg").arg(theme_name()).arg(name);
}

QString AppConfig::path(StandardPath path_type) const {
    switch (path_type) {
        case StandardPath::AppHome: {
            return QDir::cleanPath(QCoreApplication::applicationDirPath());
        } break;
        case StandardPath::UserData: {
            return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/data");
        } break;
        case StandardPath::Log: {
            return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/data/logs");
        } break;
    }
}

QString AppConfig::settings_file() const {
    return QDir::cleanPath(path(StandardPath::UserData) + "/settings.toml");
}

QString AppConfig::default_font_family() const {
    return "Sarasa Gothic SC";
}

QFont AppConfig::font(FontStyle style, int point_size) const {
    const auto family     = default_font_family();
    const auto style_name = magic_enum::enum_name(style);
    Q_ASSERT(QFontDatabase::hasFamily(family));
    Q_ASSERT(QFontDatabase::styles(family).contains(style_name.data()));
    return QFontDatabase::font(family, style_name.data(), point_size);
}

void AppConfig::set_option(Option opt, bool on) {
    if (on == option_enabled(opt)) { return; }
    if (on) {
        settings_options_.insert(opt);
    } else {
        settings_options_.remove(opt);
    }
    emit on_option_change(opt, on);
}

void AppConfig::set_value(ValOption opt, const QString& value) {
    if (value == this->value(opt)) { return; }
    settings_values_[opt] = value;
    emit on_value_change(opt, value);
}

AppConfig::Page AppConfig::primary_page() const {
    const auto page_name = value(ValOption::PrimaryPage).toLower();
    if (page_name == "gallery") {
        return Page::Gallery;
    } else if (page_name == "edit") {
        return Page::Edit;
    } else {
        Q_UNREACHABLE();
    }
}

void AppConfig::load() {
    BlockSignalGuard guard(this);

    load_default();

    const auto conf_path = settings_file();
    if (!QFile::exists(conf_path)) { return; }

    toml::table settings{};
    try {
        settings = toml::parse_file(settings_file().toLocal8Bit().toStdString());
    } catch (toml::parse_error& e) {
        spdlog::warn("failed to load settings: {}", e.what());
        return;
    } catch (...) {
        spdlog::error("unknown error when loading settings");
        QCoreApplication::quit();
    }

    for (auto&& [key, value] : settings) {
        if (false) {
        } else if (auto style = value.as_table(); style && key == "style") {
            if (auto theme = style->get_as<std::string>("theme")) {
                const auto theme_name = QString::fromStdString(theme->get()).toLower();
                for (auto theme : magic_enum::enum_values<ColorTheme>()) {
                    if (theme_name == QString{magic_enum::enum_name(theme).data()}.toLower()) {
                        set_theme(theme);
                        break;
                    }
                }
            }
            if (auto scheme = style->get_as<toml::table>("scheme")) {
                auto new_scheme = this->scheme();
                for (const auto& [role_name, role] : get_color_roles_for_settings()) {
                    if (auto argb = scheme->get_as<std::string>(role_name.toStdString())) {
                        auto new_color = QColor::fromString(argb->get());
                        if (new_color.isValid()) { new_scheme[role] = new_color; }
                    }
                }
                set_scheme(new_scheme);
            }
        } else if (auto common = value.as_table(); common && key == "common") {
            PARSE_STR_OPTION(common, Language);
            PARSE_STR_OPTION(common, PrimaryPage);
            PARSE_BOOL_OPTION(common, AutoHideToolbarOnFullscreen);
            PARSE_STR_OPTION(common, BookDirStyle);
            PARSE_STR_OPTION(common, BackgroundImage);
            PARSE_STR_OPTION(common, EditorBackgroundImage);
            PARSE_INT_OPTION(common, BackgroundImageOpacity);
            PARSE_DOUBLE_OPTION(common, LineSpacingRatio);
            PARSE_DOUBLE_OPTION(common, BlockSpacing);
            PARSE_BOOL_OPTION(common, FirstLineIndent);
            PARSE_BOOL_OPTION(common, HighlightActiveBlock);
            PARSE_BOOL_OPTION(common, ElasticTextViewResize);
            PARSE_INT_OPTION(common, TextFontSize);
            PARSE_STR_OPTION(common, DefaultEditMode);
            PARSE_BOOL_OPTION(common, StrictWordCount);
            PARSE_STR_OPTION(common, LastEditingBookOnQuit);
            PARSE_BOOL_OPTION(common, SmoothScroll);
            PARSE_STR_ARRAY_OPTION(common, TextFont);
        } else if (auto edit = value.as_table(); edit && key == "edit") {
            PARSE_BOOL_OPTION(edit, CentreEditLine);
            PARSE_BOOL_OPTION(edit, PairingSymbolMatch);
            PARSE_OPTIONAL_DOUBLE_OPTION(edit, AutoChapter, AutoChapterThreshold, threshold);
            PARSE_OPTIONAL_INT_OPTION(edit, ChapterLimit, ChapterLimit, threshold);
            PARSE_OPTIONAL_INT_OPTION(edit, CriticalChapterLimit, CriticalChapterLimit, threshold);
        } else if (auto backup = value.as_table(); backup && key == "backup") {
            PARSE_BOOL_OPTION_MANNUAL(backup, BackupSmartMerge, smart_merge);
            PARSE_BOOL_OPTION(backup, KeyVersionRecognition);
            PARSE_OPTIONAL_DOUBLE_OPTION_MANNUAL(
                backup, TimingBackup, timing_mode, TimingBackupInterval, interval);
            PARSE_OPTIONAL_INT_OPTION_MANNUAL(
                backup,
                QuantitativeBackup,
                quantitative_mode,
                QuantitativeBackupThreshold,
                threshold);
        } else if (auto dev_options = value.as_table(); dev_options && key == "dev-options") {
            PARSE_INT_OPTION(dev_options, ToolbarIconSize);
        }
    }

} // namespace jwrite

void AppConfig::save() {
    const auto conf_path = settings_file();
    QFile      settings_file(conf_path);
    if (!settings_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        spdlog::error("failed to write settings");
        return;
    }

    FullCoverageEnumVisitor<Option>    p;
    FullCoverageEnumVisitor<ValOption> q;

    const auto into_bool = [this, &p](Option opt) {
        return option_enabled(p[opt]);
    };

    const auto into_uint = [this, &q](ValOption opt) {
        bool ok = false;
        if (auto val = value(q[opt]).toUInt(&ok); ok) { return val; }
        if (auto val = default_option(q[opt]).toUInt(&ok); ok) { return val; }
        Q_UNREACHABLE();
    };

    const auto into_double = [this, &q](ValOption opt) {
        bool ok = false;
        if (auto val = value(q[opt]).toDouble(&ok); ok) { return val; }
        if (auto val = default_option(q[opt]).toDouble(&ok); ok) { return val; }
        Q_UNREACHABLE();
    };

    const auto into_str = [this, &q](ValOption opt) {
        return value(q[opt]).toStdString();
    };

    toml::table settings{};

    {
        toml::table style{};

        toml::table scheme{};
        for (const auto& [role_name, role] : get_color_roles_for_settings()) {
            const auto name  = role_name.toStdString();
            const auto color = scheme_[role].name(QColor::HexArgb).toStdString();
            scheme.insert(name, color);
        }

        style.insert("theme", theme_name().toStdString());
        style.insert("scheme", scheme);

        settings.insert("style", style);
    }

    {
        toml::table common{};

        toml::array font_famlies{};
        for (auto family : value(q[ValOption::TextFont]).split(',')) {
            if (!family.isEmpty()) { font_famlies.push_back(family.toStdString()); }
        }

        common.insert("language", into_str(ValOption::Language));
        common.insert("primary_page", into_str(ValOption::PrimaryPage));
        common.insert(
            "auto_hide_toolbar_on_fullscreen", into_bool(Option::AutoHideToolbarOnFullscreen));
        common.insert("book_dir_style", into_str(ValOption::BookDirStyle));
        common.insert("background_image", into_str(ValOption::BackgroundImage));
        common.insert("editor_background_image", into_str(ValOption::EditorBackgroundImage));
        common.insert("background_image_opacity", into_uint(ValOption::BackgroundImageOpacity));
        common.insert("line_spacing_ratio", into_double(ValOption::LineSpacingRatio));
        common.insert("block_spacing", into_double(ValOption::BlockSpacing));
        common.insert("first_line_indent", into_bool(Option::FirstLineIndent));
        common.insert("highlight_active_block", into_bool(Option::HighlightActiveBlock));
        common.insert("elastic_text_view_resize", into_bool(Option::ElasticTextViewResize));
        common.insert("text_font", font_famlies);
        common.insert("text_font_size", into_uint(ValOption::TextFontSize));
        common.insert("default_edit_mode", into_str(ValOption::DefaultEditMode));
        common.insert("strict_word_count", into_bool(Option::StrictWordCount));
        common.insert("last_editing_book_on_quit", into_str(ValOption::LastEditingBookOnQuit));
        common.insert("smooth_scroll", into_bool(Option::SmoothScroll));

        settings.insert("common", common);
    }

    {
        toml::table edit{};

        toml::table auto_chapter{};
        auto_chapter.insert("enabled", into_bool(Option::AutoChapter));
        auto_chapter.insert("threshold", into_uint(ValOption::AutoChapterThreshold));

        toml::table chapter_limit{};
        chapter_limit.insert("enabled", into_bool(Option::ChapterLimit));
        chapter_limit.insert("threshold", into_uint(ValOption::ChapterLimit));

        toml::table critical_chapter_limit{};
        critical_chapter_limit.insert("enabled", into_bool(Option::CriticalChapterLimit));
        critical_chapter_limit.insert("threshold", into_uint(ValOption::CriticalChapterLimit));

        edit.insert("centre_edit_line", into_bool(Option::CentreEditLine));
        edit.insert("pairing_symbol_match", into_bool(Option::PairingSymbolMatch));
        edit.insert("auto_chapter", auto_chapter);
        edit.insert("chapter_limit", chapter_limit);
        edit.insert("critical_chapter_limit", critical_chapter_limit);

        settings.insert("edit", edit);
    }

    {
        toml::table backup{};

        toml::table timing_mode{};
        timing_mode.insert("enabled", into_bool(Option::TimingBackup));
        timing_mode.insert("interval", into_double(ValOption::TimingBackupInterval));

        toml::table quantitative_mode{};
        quantitative_mode.insert("enabled", into_bool(Option::QuantitativeBackup));
        quantitative_mode.insert("threshold", into_uint(ValOption::QuantitativeBackupThreshold));

        backup.insert("smart_merge", into_bool(Option::BackupSmartMerge));
        backup.insert("key_version_recognition", into_bool(Option::KeyVersionRecognition));
        backup.insert("timing_mode", timing_mode);
        backup.insert("quantitative_mode", quantitative_mode);

        settings.insert("backup", backup);
    }

    {
        toml::table dev_options{};
        dev_options.insert("toolbar_icon_size", into_uint(ValOption::ToolbarIconSize));
        settings.insert("dev-options", dev_options);
    }

    std::stringstream ss;

    using fmt   = toml::toml_formatter;
    using flags = toml::format_flags;
    ss << fmt(settings, fmt::default_flags & ~flags::indentation) << "\n";

    QTextStream stream(&settings_file);
    stream << ss.str().c_str();
    settings_file.close();
}

AppConfig& AppConfig::get_instance() {
    static std::unique_ptr<AppConfig> instance{nullptr};
    if (!instance) { instance = std::make_unique<AppConfig>(); }
    return *instance;
}

bool AppConfig::default_option(Option opt) {
    static QMap<Option, bool> DEFAULT_OPTIONS{
        {Option::AutoHideToolbarOnFullscreen, true },
        {Option::FirstLineIndent,             true },
        {Option::HighlightActiveBlock,        true },
        {Option::ElasticTextViewResize,       false},
        {Option::CentreEditLine,              false},
        {Option::AutoChapter,                 false},
        {Option::PairingSymbolMatch,          true },
        {Option::CriticalChapterLimit,        false},
        {Option::ChapterLimit,                false},
        {Option::TimingBackup,                false},
        {Option::QuantitativeBackup,          false},
        {Option::BackupSmartMerge,            false},
        {Option::KeyVersionRecognition,       false},
        {Option::StrictWordCount,             true },
        {Option::SmoothScroll,                true },
    };
    Q_ASSERT(DEFAULT_OPTIONS.size() == magic_enum::enum_count<Option>());
    return DEFAULT_OPTIONS.value(opt);
}

QString AppConfig::default_option(ValOption opt) {
    static QMap<ValOption, QString> DEFAULT_OPTIONS{
        {ValOption::Language,                    "zh_CN"  },
        {ValOption::PrimaryPage,                 "gallery"},
        {ValOption::DefaultEditMode,             "normal" },
        {ValOption::TextFont,                    ""       },
        {ValOption::TextFontSize,                "16"     },
        {ValOption::LineSpacingRatio,            "1.0"    },
        {ValOption::BlockSpacing,                "6"      },
        {ValOption::BookDirStyle,                "tree"   },
        {ValOption::AutoChapterThreshold,        "2000"   },
        {ValOption::CriticalChapterLimit,        "20"     },
        {ValOption::ChapterLimit,                "100"    },
        {ValOption::TimingBackupInterval,        "5.0"    },
        {ValOption::QuantitativeBackupThreshold, "100"    },
        {ValOption::BackgroundImage,             ""       },
        {ValOption::EditorBackgroundImage,       ""       },
        {ValOption::BackgroundImageOpacity,      "100"    },
        {ValOption::ToolbarIconSize,             "12"     },
        {ValOption::LastEditingBookOnQuit,       ""       },
    };
    Q_ASSERT(DEFAULT_OPTIONS.size() == magic_enum::enum_count<ValOption>());
    return DEFAULT_OPTIONS.value(opt);
}

void AppConfig::load_default() {
    for (const auto& opt : magic_enum::enum_values<Option>()) {
        set_option(opt, default_option(opt));
    }

    for (const auto& opt : magic_enum::enum_values<ValOption>()) {
        set_value(opt, default_option(opt));
    }

    const auto default_theme = ColorTheme::Dark;
    set_theme(default_theme);
    set_scheme(ColorScheme::get_default_scheme_of_theme(default_theme));
}

} // namespace jwrite
