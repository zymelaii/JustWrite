#pragma once

#include <jwrite/ColorScheme.h>
#include <QObject>
#include <QMap>
#include <QSet>

namespace jwrite {

class AppConfig : public QObject {
    Q_OBJECT

public:
    enum class StandardPath {
        AppHome,
        UserData,
        Log,
    };

    enum class FontStyle {
        Light,
        Regular,
        Bold,
        Italic,
    };

    enum class Option {
        AutoHideToolbarOnFullscreen,
        FirstLineIndent,
        ElasticTextViewResize,
        AutoChapter,
        PairingSymbolMatch,
        CriticalChapterLimit,
        ChapterLimit,
        TimingBackup,
        QuantitativeBackup,
        BackupSmartMerge,
        KeyVersionRecognition,
        StrictWordCount,
        SmoothScroll,
    };

    enum class ValOption {
        Language,
        PrimaryPage,
        DefaultEditMode,
        TextFont,
        //! in units of point
        TextFontSize,
        LineSpacingRatio,
        //! in units of pixel
        BlockSpacing,
        BookDirStyle,
        AutoChapterThreshold,
        //! in units of ten thousand
        CriticalChapterLimit,
        //! in units of thousand
        ChapterLimit,
        //! in units of minute
        TimingBackupInterval,
        QuantitativeBackupThreshold,
        BackgroundImage,
        EditorBackgroundImage,
        //! in units of percentage
        BackgroundImageOpacity,
        //! in units of pixel
        ToolbarIconSize,
        LastEditingBookOnQuit,
        UnfocusedTextOpacity,
        TextFocusMode,
        CentreEditLine,
    };

    enum class Page {
        Gallery,
        Edit,
    };

    enum class TextFocusMode {
        None,
        Highlight,
        FocusLine,
        FocusBlock,
    };

public:
    AppConfig();
    ~AppConfig() override;

signals:
    void on_theme_change(ColorTheme theme);
    void on_scheme_change(const ColorScheme& scheme);
    void on_option_change(Option opt, bool on);
    void on_value_change(ValOption opt, const QString& value);

public:
    QString    theme_name() const;
    ColorTheme theme() const;
    void       set_theme(ColorTheme theme);

    const ColorScheme& scheme() const;
    void               set_scheme(const ColorScheme& scheme);

    QString icon(const QString& name) const;
    QString path(StandardPath path_type) const;
    QString settings_file() const;

    QString default_font_family() const;
    QFont   font(FontStyle style, int point_size) const;

    bool option_enabled(Option opt) const;
    void set_option(Option opt, bool on);

    QString value(ValOption opt) const;
    void    set_value(ValOption opt, const QString& value);

    void load();
    void save();

    Page          primary_page() const;
    TextFocusMode text_focus_mode() const;

    static Page          get_primary_page_from_name(const QString& name);
    static TextFocusMode get_text_focus_mode_name(const QString& name);

    static AppConfig& get_instance();
    static bool       default_option(Option opt);
    static QString    default_option(ValOption opt);

protected:
    void load_default();

private:
    QMap<ValOption, QString> settings_values_;
    QSet<Option>             settings_options_;
    ColorTheme               theme_;
    ColorScheme              scheme_;
};

inline ColorTheme AppConfig::theme() const {
    return theme_;
}

inline const ColorScheme& AppConfig::scheme() const {
    return scheme_;
}

inline bool AppConfig::option_enabled(Option opt) const {
    return settings_options_.contains(opt);
}

inline QString AppConfig::value(ValOption opt) const {
    //! FIXME: consider update to default value if not found
    return settings_values_.value(opt, default_option(opt));
}

} // namespace jwrite
