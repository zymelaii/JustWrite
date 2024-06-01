#pragma once

#include <jwrite/ColorScheme.h>
#include <QDialog>

namespace jwrite::ui {

class ColorSchemeDialog : public QDialog {
    Q_OBJECT

signals:
    void on_scheme_change(ColorTheme theme, const ColorScheme &scheme);
    void on_request_apply(ColorTheme theme, const ColorScheme &scheme);

public:
    ColorTheme theme() const {
        return theme_;
    }

    const ColorScheme &scheme() const {
        Q_ASSERT(schemes_.contains(theme_));
        return const_cast<ColorSchemeDialog *>(this)->schemes_[theme_];
    }

public:
    explicit ColorSchemeDialog(
        ColorTheme initial_theme, const ColorScheme &initial_scheme, QWidget *parent = nullptr);

protected:
    void init();

private:
    ColorTheme                    theme_;
    QMap<ColorTheme, ColorScheme> schemes_;
};

} // namespace jwrite::ui
