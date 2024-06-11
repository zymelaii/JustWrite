#pragma once

#include <widget-kit/OverlayDialog.h>
#include <widget-kit/FlatButton.h>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <optional>

namespace jwrite::ui {

class FontSelectDialog : public widgetkit::OverlayDialog {
    Q_OBJECT

public slots:
    void add_font(const QString &family);

public:
    static std::optional<QStringList>
        get_font_families(widgetkit::OverlaySurface *surface, QStringList initial_families);

public:
    FontSelectDialog();
    ~FontSelectDialog() override;

protected:
    void init();

    void paintEvent(QPaintEvent *event) override;

private:
    QStringList font_families_;

    QVBoxLayout           *ui_layout_;
    QVBoxLayout           *ui_font_layout_;
    QComboBox             *ui_font_combo_;
    widgetkit::FlatButton *ui_commit_;
    QMap<QString, QFont>   ui_fonts_;
};

} // namespace jwrite::ui
