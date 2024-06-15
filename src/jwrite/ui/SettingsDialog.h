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
    void on_request_editor_font_select();
    void on_request_color_scheme_editor();
    void on_request_background_image_picker();
    void on_request_editor_background_image_picker();
    void on_request_check_update();

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
