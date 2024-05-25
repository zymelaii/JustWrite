#pragma once

#include <jwrite/ColorTheme.h>
#include <QComboBox>
#include <QDialog>

namespace jwrite::ui {

class ColorThemeDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColorThemeDialog(QWidget *parent = nullptr);
    explicit ColorThemeDialog(ColorTheme theme, QWidget *parent = nullptr);

    ~ColorThemeDialog();

signals:
    void themeChanged();
    void themeApplied();

public:
    ColorTheme getTheme() const {
        return theme_;
    }

protected slots:
    void notifyThemeChanged(int schema_index);
    void notifyThemeApplied();

protected:
    void setupUi();

private:
    ColorTheme theme_;

    QComboBox *ui_schema_select_;
};

} // namespace jwrite::ui
