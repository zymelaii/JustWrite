#pragma once

#include <jwrite/ColorScheme.h>
#include <QDialog>

namespace jwrite::ui {

class ColorSchemeDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColorSchemeDialog(
        ColorTheme initial_theme, const ColorScheme &initial_scheme, QWidget *parent = nullptr);
    ~ColorSchemeDialog() override;

signals:
    void schemeChanged(ColorTheme theme, const ColorScheme &scheme);
    void applyRequested(ColorTheme theme, const ColorScheme &scheme);

public:
    ColorTheme getTheme() const {
        return theme_;
    }

    const ColorScheme &getScheme() const {
        Q_ASSERT(schemes_.contains(theme_));
        return const_cast<ColorSchemeDialog *>(this)->schemes_[theme_];
    }

protected:
    void setupUi();

private:
    ColorTheme                    theme_;
    QMap<ColorTheme, ColorScheme> schemes_;
};

} // namespace jwrite::ui
