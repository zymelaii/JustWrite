#pragma once

#include <jwrite/ui/JustWrite.h>
#include <widget-kit/OverlayDialog.h>
#include <widget-kit/FlatButton.h>
#include <QScrollArea>

namespace jwrite::ui {

class SettingsDialog : public widgetkit::OverlayDialog {
    Q_OBJECT

public:
    enum SettingsPanel {
        SP_Normal,
        SP_Editor,
        SP_Appearance,
        SP_Backup,
        SP_Statistics,
        SP_Shortcut,
        SP_About,
        SP_Develop,
        SP_DeepCustomize,
    };

private:
    struct PanelLink {
        widgetkit::FlatButton *button;
        QWidget               *panel;
    };

signals:
    void on_primary_page_change(JustWrite::PageType page);
    void on_language_change(const QString &lang);
    void on_editor_default_edit_mode_change(int mode);
    void on_editor_font_change(const QString &font);
    void on_editor_font_size_change(int size);
    void on_editor_first_line_indent_change(bool enabled);
    void on_editor_line_spacing_change(double ratio);
    void on_editor_block_spacing_change(double spacing);
    void on_editor_highlight_active_block_change(bool enabled);
    void on_editor_elastic_resize_change(bool enabled);
    void on_editor_centre_edit_line_change(bool enabled);
    void on_editor_auto_chapter_change(int limit);
    void on_editor_pairing_symbol_match_change(bool enabled);
    void on_editor_book_dir_style_change(int style);
    void on_editor_chapter_critical_limit_change(int limit);
    void on_editor_chapter_limit_change(int limit);
    void on_color_theme_change(ColorTheme theme);
    void on_request_color_scheme_editor();
    void on_color_scheme_change(const ColorScheme &scheme);
    void on_request_background_image_picker();
    void on_background_image_change(const QString &path);
    void on_request_editor_background_image_picker();
    void on_editor_background_image_change(const QString &path);
    void on_backgroup_image_opacity_change(int opacity);
    void on_auto_hide_toolbar_change(bool enabled);
    void on_backup_timing_backup_change(int interval);
    void on_backup_quantitative_backup_change(int limit);
    void on_backup_smart_merge_change(bool enabled);
    void on_backup_key_version_recognize_change(bool enabled);
    void on_strict_word_count_change(bool enabled);
    void on_global_command_shortcut_change(GlobalCommand command, QKeySequence shortcut);
    void on_text_input_command_shortcut_change(TextInputCommand command, QKeySequence shortcut);
    void on_request_check_update();
    void on_toolbar_icon_size_change(int size);

public:
    void handle_on_switch_to_panel(SettingsPanel panel);

    int         exec(widgetkit::OverlaySurface *surface) override;
    static void show(widgetkit::OverlaySurface *surface);

public:
    SettingsDialog();

protected:
    void init();

    [[nodiscard]] QWidget *createNormalPanel();
    [[nodiscard]] QWidget *createEditorPanel();
    [[nodiscard]] QWidget *createAppearancePanel();
    [[nodiscard]] QWidget *createBackupPanel();
    [[nodiscard]] QWidget *createStatisticsPanel();
    [[nodiscard]] QWidget *createShortcutPanel();
    [[nodiscard]] QWidget *createAboutPanel();
    [[nodiscard]] QWidget *createDevelopPanel();
    [[nodiscard]] QWidget *createDeepCustomizePanel();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QMap<SettingsPanel, PanelLink> panels_;

    QScrollArea *ui_panel_area_;
};

} // namespace jwrite::ui
