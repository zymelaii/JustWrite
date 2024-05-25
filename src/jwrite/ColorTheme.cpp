#include <jwrite/ColorTheme.h>

namespace jwrite {

ColorTheme ColorTheme::from_schema(ColorSchema schema) {
    ColorTheme theme{};
    switch (schema) {
        case ColorSchema::Light: {
            theme.Window       = QColor(255, 255, 255);
            theme.WindowText   = QColor(23, 24, 27);
            theme.Border       = QColor(200, 200, 200);
            theme.Panel        = QColor(228, 228, 215);
            theme.Text         = QColor(31, 32, 33);
            theme.TextBase     = QColor(241, 223, 222);
            theme.SelectedText = QColor(140, 180, 100);
            theme.Hover        = QColor(30, 30, 80, 50);
            theme.SelectedItem = QColor(196, 165, 146, 60);
        } break;
        case ColorSchema::Dark: {
            theme.Window       = QColor(60, 60, 60);
            theme.WindowText   = QColor(160, 160, 160);
            theme.Border       = QColor(70, 70, 70);
            theme.Panel        = QColor(38, 38, 38);
            theme.Text         = QColor(255, 255, 255);
            theme.TextBase     = QColor(30, 30, 30);
            theme.SelectedText = QColor(60, 60, 255, 80);
            theme.Hover        = QColor(255, 255, 255, 30);
            theme.SelectedItem = QColor(255, 255, 255, 10);
        } break;
    }
    return theme;
}

} // namespace jwrite
