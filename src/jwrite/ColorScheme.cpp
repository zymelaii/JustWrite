#include <jwrite/ColorScheme.h>

namespace jwrite {

ColorScheme ColorScheme::get_default_scheme_of_theme(ColorTheme theme) {
    ColorScheme scheme{};
    switch (theme) {
        case ColorTheme::Light: {
            scheme[ColorRole::Window]             = QColor(255, 255, 255);
            scheme[ColorRole::WindowText]         = QColor(23, 24, 27);
            scheme[ColorRole::Border]             = QColor(200, 200, 200);
            scheme[ColorRole::Panel]              = QColor(228, 228, 215);
            scheme[ColorRole::Text]               = QColor(31, 32, 33);
            scheme[ColorRole::TextBase]           = QColor(241, 223, 222);
            scheme[ColorRole::SelectedText]       = QColor(140, 180, 100, 80);
            scheme[ColorRole::Hover]              = QColor(30, 30, 80, 30);
            scheme[ColorRole::SelectedItem]       = QColor(196, 165, 146, 50);
            scheme[ColorRole::FloatingItem]       = QColor(201, 238, 213, 200);
            scheme[ColorRole::FloatingItemBorder] = QColor(245, 245, 245);
        } break;
        case ColorTheme::Dark: {
            scheme[ColorRole::Window]             = QColor(60, 60, 60);
            scheme[ColorRole::WindowText]         = QColor(160, 160, 160);
            scheme[ColorRole::Border]             = QColor(70, 70, 70);
            scheme[ColorRole::Panel]              = QColor(38, 38, 38);
            scheme[ColorRole::Text]               = QColor(255, 255, 255);
            scheme[ColorRole::TextBase]           = QColor(30, 30, 30);
            scheme[ColorRole::SelectedText]       = QColor(60, 60, 255, 80);
            scheme[ColorRole::Hover]              = QColor(255, 255, 255, 30);
            scheme[ColorRole::SelectedItem]       = QColor(255, 255, 255, 10);
            scheme[ColorRole::FloatingItem]       = QColor(66, 66, 66, 200);
            scheme[ColorRole::FloatingItemBorder] = QColor(40, 40, 40);
        } break;
    }
    return scheme;
}

} // namespace jwrite
