#include <jwrite/ui/JustWrite.h>
#include <jwrite/ColorTheme.h>
#include <jwrite/ProfileUtils.h>
#include <jwrite/ui/ScrollArea.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QDateTime>
#include <QPainter>
#include <QMessageBox>
#include <QScrollArea>

namespace jwrite::Ui {

JustWrite::JustWrite(QWidget *parent)
    : QWidget(parent) {
    setupUi();

    switchToPage(PageType::Gallery);
    setTheme(Theme::Dark);
}

JustWrite::~JustWrite() {
    jwrite_profiler_dump(QString("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss")));
}

void JustWrite::setTheme(Theme theme) {
    ColorTheme color_theme;

    switch (theme) {
        case Theme::Light: {
            color_theme.Window       = QColor(255, 255, 255);
            color_theme.WindowText   = QColor(23, 24, 27);
            color_theme.Border       = QColor(200, 200, 200);
            color_theme.Panel        = QColor(228, 228, 215);
            color_theme.Text         = QColor(31, 32, 33);
            color_theme.TextBase     = QColor(241, 223, 222);
            color_theme.SelectedText = QColor(140, 180, 100);
            color_theme.Hover        = QColor(30, 30, 80, 50);
            color_theme.SelectedItem = QColor(196, 165, 146, 60);
        } break;
        case Theme::Dark: {
            color_theme.Window       = QColor(60, 60, 60);
            color_theme.WindowText   = QColor(160, 160, 160);
            color_theme.Border       = QColor(70, 70, 70);
            color_theme.Panel        = QColor(38, 38, 38);
            color_theme.Text         = QColor(255, 255, 255);
            color_theme.TextBase     = QColor(30, 30, 30);
            color_theme.SelectedText = QColor(60, 60, 255, 80);
            color_theme.Hover        = QColor(255, 255, 255, 30);
            color_theme.SelectedItem = QColor(255, 255, 255, 10);
        } break;
    }

    auto pal = palette();
    pal.setColor(QPalette::Window, color_theme.Window);
    pal.setColor(QPalette::WindowText, color_theme.WindowText);
    pal.setColor(QPalette::Base, color_theme.TextBase);
    pal.setColor(QPalette::Text, color_theme.Text);
    pal.setColor(QPalette::Highlight, color_theme.SelectedText);
    pal.setColor(QPalette::Button, color_theme.Window);
    pal.setColor(QPalette::ButtonText, color_theme.WindowText);
    setPalette(pal);

    ui_gallery->updateColorTheme(color_theme);
    ui_edit_page->updateColorTheme(color_theme);
}

void JustWrite::setupUi() {
    auto top_layout    = new QVBoxLayout(this);
    auto container     = new QWidget;
    ui_top_most_layout = new QStackedLayout(container);

    ui_title_bar = new jwrite::Ui::TitleBar;

    top_layout->addWidget(ui_title_bar);
    top_layout->addWidget(container);

    ui_page_stack  = new QStackedWidget;
    ui_gallery     = new jwrite::Ui::Gallery;
    ui_edit_page   = new jwrite::Ui::EditPage;
    ui_popup_layer = new QWidget;

    auto gallery_page = new jwrite::ui::ScrollArea;
    gallery_page->setWidget(ui_gallery);

    ui_page_stack->addWidget(gallery_page);
    ui_page_stack->addWidget(ui_edit_page);

    ui_top_most_layout->addWidget(ui_page_stack);
    ui_top_most_layout->addWidget(ui_popup_layer);

    ui_agent = new QWK::WidgetWindowAgent(this);
    ui_agent->setup(this);
    ui_agent->setTitleBar(ui_title_bar);

    top_layout->setContentsMargins({});
    top_layout->setSpacing(0);

    container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    ui_top_most_layout->setStackingMode(QStackedLayout::StackAll);

    page_map_[PageType::Gallery] = gallery_page;
    page_map_[PageType::Edit]    = ui_edit_page;

    ui_popup_layer->hide();

    ui_title_bar->setTitle("只写 丶 阐释你的梦");
}

void JustWrite::switchToPage(PageType page) {
    Q_ASSERT(page_map_.contains(page));
    Q_ASSERT(page_map_.value(page, nullptr));
    ui_page_stack->setCurrentWidget(page_map_[page]);
}

} // namespace jwrite::Ui
