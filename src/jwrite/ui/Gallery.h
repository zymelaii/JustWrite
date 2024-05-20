#pragma once

#include <jwrite/ColorTheme.h>
#include <jwrite/BookInfo.h>
#include <QWidget>
#include <QUrl>

namespace jwrite::ui {

class Gallery : public QWidget {
    Q_OBJECT

public:
    enum MenuAction {
        Open,
        Edit,
        Delete,
    };

public:
    explicit Gallery(QWidget *parent = nullptr);
    ~Gallery();

signals:
    void clicked(int index);
    void menuClicked(int index, MenuAction action);

public:
    void removeDisplayCase(int index);
    void updateDisplayCaseItem(int index, const QString &title, const QString &cover_url);
    void updateColorTheme(const ColorTheme &color_theme);

    int totalItems() const {
        return items_.size();
    }

    jwrite::BookInfo bookInfoAt(int index) const {
        //! TODO: record book id
        Q_ASSERT(index >= 0 && index < items_.size());
        const auto &item = items_[index];
        return {
            .title     = item.title,
            .author    = "",
            .cover_url = item.source,
        };
    }

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool hasHeightForWidth() const override {
        return true;
    }

    int heightForWidth(int width) const override;

protected:
    void  setupUi();
    void  resetHoverState();
    int   getTitleHeight() const;
    int   getTitleOffset() const;
    QRect getItemRect(int row, int col) const;
    QRect getItemTitleRect(const QRect &item_rect) const;
    int   getItemIndex(const QPoint &pos, int *out_menu_index) const;
    QRect getDisplayCaseMenuButtonRect(const QRect &cover_bb, int &out_v_spacing) const;
    void  drawDisplayCase(QPainter *p, int index, int row, int col);
    void  drawDisplayCaseMenu(QPainter *p, int index, const QRect &cover_bb);

    void paintEvent(QPaintEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct DisplayCaseItem {
        QString source;
        QPixmap cover;
        QString title;
    };

    QList<DisplayCaseItem> items_;
    int                    hover_index_;
    int                    hover_btn_index_;
    QSize                  item_size_;
    int                    item_spacing_;
};

} // namespace jwrite::ui
