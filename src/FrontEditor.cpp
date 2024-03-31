#include "FrontEditor.h"
#include "ParaBlockEdit.h"
#include <QVBoxLayout>
#include <QPalette>
#include <QVector>
#include <QFocusEvent>

struct FrontEditorPrivate {
    QVBoxLayout *layout;

    QVector<ParaBlockEdit *> para_pool;
    QVector<ParaBlockEdit *> active_para_blocks;
    int                      active_para;
    int                      last_active_para;

    FrontEditorPrivate() {
        layout           = nullptr;
        active_para      = -1;
        last_active_para = active_para;
    }
};

FrontEditor::FrontEditor(QWidget *parent)
    : QWidget(parent)
    , d{new FrontEditorPrivate} {
    setFocusPolicy(Qt::StrongFocus);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(40, 40, 40));
    setPalette(pal);

    setAutoFillBackground(true);

    d->layout = new QVBoxLayout(this);
    d->layout->addStretch();
}

FrontEditor::~FrontEditor() {
    delete d;
}

void FrontEditor::enterNextParaBlock() {
    if (d->active_para != -1) {
        auto current_para = d->active_para_blocks[d->active_para];
        if (current_para->text().isEmpty()) { return; }
    }
    auto para  = createParaBlock();
    int  index = d->active_para == -1 ? 0 : d->active_para + 1;
    d->active_para_blocks.insert(index, para);
    d->layout->insertWidget(index, para);
    setActiveParaBlock(index);
}

ParaBlockEdit *FrontEditor::createParaBlock() {
    ParaBlockEdit *para = nullptr;
    if (!d->para_pool.empty()) {
        para = d->para_pool.back();
        d->para_pool.pop_back();
        para->clearText();
        para->show();
    } else {
        para = new ParaBlockEdit;
    }
    connect(para, &ParaBlockEdit::finished, this, &FrontEditor::enterNextParaBlock);
    connect(para, &ParaBlockEdit::gotFocus, this, [this, para] {
        trackActiveParaBlock(para);
    });
    connect(para, &ParaBlockEdit::lostFocus, this, [this, para] {
        trackInactiveParaBlock(para);
    });
    return para;
}

void FrontEditor::setActiveParaBlock(int index) {
    if (index < 0 || index >= d->active_para_blocks.size()) { return; }
    auto para           = d->active_para_blocks[index];
    d->last_active_para = d->active_para;
    d->active_para      = index;
    para->setFocus();
}

void FrontEditor::trackActiveParaBlock(ParaBlockEdit *target) {
    int index = d->active_para_blocks.indexOf(target);
    if (index == -1) { return; }
    d->active_para = index;
}

void FrontEditor::trackInactiveParaBlock(ParaBlockEdit *target) {
    int index = d->active_para_blocks.indexOf(target);
    if (index == -1) { return; }
    if (!target->text().isEmpty()) { return; }

    Q_ASSERT(d->active_para == index);
    d->last_active_para = d->active_para;
    d->active_para      = -1;

    auto w = d->layout->takeAt(index)->widget();
    Q_ASSERT(w);
    auto para = static_cast<ParaBlockEdit *>(w);
    para->hide();
    d->para_pool.push_back(para);
    d->active_para_blocks.remove(index);

    disconnect(para, SIGNAL(finished()));
    disconnect(para, SIGNAL(gotFocus()));
    disconnect(para, SIGNAL(lostFocus()));
}

void FrontEditor::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);
    if (d->active_para_blocks.empty()) {
        enterNextParaBlock();
    } else if (d->active_para == -1) {
        int active_para = d->last_active_para == -1 ? 0 : d->last_active_para;
        setActiveParaBlock(active_para);
    } else {
        setActiveParaBlock(d->active_para);
    }
}

void FrontEditor::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    d->last_active_para = d->active_para;
    d->active_para      = -1;
}
