#pragma once

#include <QColor>
#include <array>
#include <magic_enum.hpp>

namespace jwrite {

enum class ColorTheme {
    Light,
    Dark,
};

class ColorScheme {
public:
    enum ColorRole {
        //! default background color
        Window,
        //! default foreground color
        WindowText,
        //! color for the border
        Border,
        //! background color for the panel-like widget
        Panel,
        //! foreground color for the text entry widget
        Text,
        //! background color for the text entry widget
        TextBase,
        //! background color for the selected text in the text entry widget
        SelectedText,
        //! background color for the item on hover
        Hover,
        //! background color for the selected item
        SelectedItem,
    };

public:
    QColor& operator[](ColorRole role) {
        Q_ASSERT(role >= 0 && role < colors_.size());
        return colors_[role];
    }

    const QColor& operator[](ColorRole role) const {
        Q_ASSERT(role >= 0 && role < colors_.size());
        return colors_[role];
    }

    const QColor& window() const {
        return colors_[ColorRole::Window];
    }

    const QColor& window_text() const {
        return colors_[ColorRole::WindowText];
    }

    const QColor& border() const {
        return colors_[ColorRole::Border];
    }

    const QColor& panel() const {
        return colors_[ColorRole::Panel];
    }

    const QColor& text() const {
        return colors_[ColorRole::Text];
    }

    const QColor& text_base() const {
        return colors_[ColorRole::TextBase];
    }

    const QColor& selected_text() const {
        return colors_[ColorRole::SelectedText];
    }

    const QColor& hover() const {
        return colors_[ColorRole::Hover];
    }

    const QColor& selected_item() const {
        return colors_[ColorRole::SelectedItem];
    }

    static ColorScheme get_default_scheme_of_theme(ColorTheme theme);

private:
    std::array<QColor, magic_enum::enum_count<ColorRole>()> colors_;
};

}; // namespace jwrite
