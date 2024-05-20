#include <jwrite/ui/JustWrite.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/BookInfoEdit.h>
#include <jwrite/ColorTheme.h>
#include <jwrite/ProfileUtils.h>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QDateTime>
#include <QPainter>

namespace jwrite::Ui {

JustWrite::JustWrite() {
    setupUi();
    setupConnections();
    closePopupLayer();

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

    ui_gallery_->updateColorTheme(color_theme);
    ui_edit_page_->updateColorTheme(color_theme);
}

void JustWrite::setupUi() {
    auto top_layout     = new QVBoxLayout(this);
    auto container      = new QWidget;
    ui_top_most_layout_ = new QStackedLayout(container);

    ui_title_bar_ = new jwrite::Ui::TitleBar;

    top_layout->addWidget(ui_title_bar_);
    top_layout->addWidget(container);

    ui_page_stack_  = new QStackedWidget;
    ui_gallery_     = new jwrite::Ui::Gallery;
    ui_edit_page_   = new jwrite::Ui::EditPage;
    ui_popup_layer_ = new jwrite::ui::FloatingDialog;

    auto gallery_page = new jwrite::ui::ScrollArea;
    gallery_page->setWidget(ui_gallery_);

    ui_page_stack_->addWidget(gallery_page);
    ui_page_stack_->addWidget(ui_edit_page_);

    ui_top_most_layout_->addWidget(ui_page_stack_);
    ui_top_most_layout_->addWidget(ui_popup_layer_);

    ui_agent_ = new QWK::WidgetWindowAgent(this);
    ui_agent_->setup(this);
    ui_agent_->setTitleBar(ui_title_bar_);

    top_layout->setContentsMargins({});
    top_layout->setSpacing(0);

    container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    ui_top_most_layout_->setStackingMode(QStackedLayout::StackAll);
    ui_top_most_layout_->setCurrentWidget(ui_popup_layer_);

    page_map_[PageType::Gallery] = gallery_page;
    page_map_[PageType::Edit]    = ui_edit_page_;

    ui_title_bar_->setTitle("只写 丶 阐释你的梦");
}

void JustWrite::setupConnections() {
    connect(ui_gallery_, &jwrite::Ui::Gallery::clicked, this, [this](int index) {
        qDebug() << "called on" << index << "total items" << ui_gallery_->totalItems();
        if (index != ui_gallery_->totalItems()) { return; }
        showPopupLayer(new jwrite::ui::BookInfoEdit);
    });
}

void JustWrite::showPopupLayer(QWidget *widget) {
    ui_popup_layer_->setWidget(widget);
    ui_popup_layer_->show();
}

void JustWrite::closePopupLayer() {
    ui_popup_layer_->hide();
    if (auto w = ui_popup_layer_->take()) { delete w; }
}

void JustWrite::switchToPage(PageType page) {
    Q_ASSERT(page_map_.contains(page));
    Q_ASSERT(page_map_.value(page, nullptr));
    if (auto w = page_map_[page]; w != ui_page_stack_->currentWidget()) {
        closePopupLayer();
        ui_page_stack_->setCurrentWidget(w);
    }
}

} // namespace jwrite::Ui
