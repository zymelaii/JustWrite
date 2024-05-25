#pragma once

#include <QColor>

namespace jwrite {

enum class ColorSchema {
    Light,
    Dark,
};

struct ColorTheme {
    //! default background color
    QColor Window;
    //! default foreground color
    QColor WindowText;
    //! color for the border
    QColor Border;
    //! background color for the panel-like widget
    QColor Panel;
    //! foreground color for the text entry widget
    QColor Text;
    //! background color for the text entry widget
    QColor TextBase;
    //! background color for the selected text in the text entry widget
    QColor SelectedText;
    //! background color for the item on hover
    QColor Hover;
    //! background color for the selected item
    QColor SelectedItem;

    static ColorTheme from_schema(ColorSchema schema);
};

}; // namespace jwrite
