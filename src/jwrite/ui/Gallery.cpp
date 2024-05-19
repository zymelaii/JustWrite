#include <jwrite/ui/Gallery.h>

namespace jwrite::Ui {

Gallery::Gallery(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

Gallery::~Gallery() {}

void Gallery::setupUi() {}

void Gallery::paintEvent(QPaintEvent *event) {}

} // namespace jwrite::Ui
