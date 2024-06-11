#pragma once

class QLayout;
class QWidget;

namespace widgetkit {

void force_update_geometry(QWidget *widget);
void force_redo_layout(QLayout *layout);

} // namespace widgetkit
