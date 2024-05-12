#include "JustWriteSidebar.h"
#include "FlatButton.h"
#include "CatalogTree.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace Ui {
struct JustWriteSidebar {
    FlatButton    *new_volume;
    FlatButton    *new_chapter;
    QLabel        *book_icon;
    QLabel        *book_title;
    ::CatalogTree *book_dir;

    JustWriteSidebar(::JustWriteSidebar *parent) {
        new_volume  = new FlatButton;
        new_chapter = new FlatButton;
        // book_icon   = new QLabel;
        // book_title  = new QLabel;
        book_dir = new ::CatalogTree;

        auto new_btn_line        = new QWidget;
        auto new_btn_line_layout = new QHBoxLayout(new_btn_line);
        new_btn_line_layout->setContentsMargins({});
        new_btn_line_layout->addStretch();
        new_btn_line_layout->addWidget(new_volume);
        new_btn_line_layout->addStretch();
        new_btn_line_layout->addWidget(new_chapter);
        new_btn_line_layout->addStretch();

        // auto title_line        = new QWidget;
        // auto title_line_layout = new QHBoxLayout(title_line);
        // title_line_layout->addWidget(book_icon);
        // title_line_layout->addWidget(book_title);
        // title_line_layout->addStretch();

        auto sep_line = new QFrame;
        sep_line->setFrameShape(QFrame::HLine);

        auto top_layout = new QVBoxLayout(parent);
        top_layout->setContentsMargins(0, 10, 0, 10);

        top_layout->addWidget(new_btn_line);
        top_layout->addWidget(sep_line);
        // top_layout->addWidget(title_line);
        top_layout->addWidget(book_dir);

        auto font = parent->font();
        font.setPointSize(10);
        new_volume->setFont(font);
        new_chapter->setFont(font);
        // font.setPointSize(13);
        // book_title->setFont(font);

        auto pal = parent->palette();
        pal.setColor(QPalette::WindowText, QColor(70, 70, 70));
        sep_line->setPalette(pal);

        // book_icon->setPixmap(QPixmap(":/res/icons/light/book-2-line.png"));
        // book_title->setText(QObject::tr("未命名"));

        new_volume->setText(QObject::tr("新建卷"));
        new_volume->setTextAlignment(Qt::AlignCenter);
        new_volume->setContentsMargins(10, 0, 10, 0);
        new_volume->setRadius(6);
        new_chapter->setText(QObject::tr("新建章"));
        new_chapter->setTextAlignment(Qt::AlignCenter);
        new_chapter->setContentsMargins(10, 0, 10, 0);
        new_chapter->setRadius(6);
    }
};
}; // namespace Ui

JustWriteSidebar::JustWriteSidebar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::JustWriteSidebar(this)) {
    setAutoFillBackground(true);
    setMinimumWidth(200);

    connect(ui->new_volume, &FlatButton::pressed, this, [this] {
        newVolume(-1, "");
    });
    connect(ui->new_chapter, &FlatButton::pressed, this, [this] {
        newChapter(-1, "");
    });
    connect(ui->book_dir, &CatalogTree::itemClicked, this, [this](int vid, int cid) {
        emit chapterOpened(cid);
    });
}

JustWriteSidebar::~JustWriteSidebar() {
    delete ui;
}

void JustWriteSidebar::newVolume(int index, const QString &title) {
    if (index == -1) { index = ui->book_dir->totalVolumes(); }
    ui->book_dir->addVolume(index, title);
}

void JustWriteSidebar::newChapter(int volume_index, const QString &title) {
    if (volume_index == -1) { volume_index = ui->book_dir->totalVolumes() - 1; }
    const auto vid = ui->book_dir->volumeAt(volume_index);
    Q_ASSERT(vid != -1);
    int cid = ui->book_dir->addChapter(vid, title);
    emit chapterOpened(cid);
}

void JustWriteSidebar::openEmptyChapter() {
    const auto vid = ui->book_dir->volumeAt(0);
    if (vid == -1) { newVolume(0, ""); }
    newChapter(ui->book_dir->totalVolumes() - 1, "");
}
