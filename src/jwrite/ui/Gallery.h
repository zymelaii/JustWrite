#pragma once

#include <jwrite/ColorTheme.h>
#include <QWidget>
#include <QUrl>

namespace jwrite::Ui {

class Gallery : public QWidget {
    Q_OBJECT

public:
    explicit Gallery(QWidget *parent = nullptr);
    ~Gallery();

public:
    void addDisplayCaseItem();
    void updateDisplayCaseItem(int index, const QString &title, const QUrl &cover_url);
    void updateColorTheme(const ColorTheme &color_theme);

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    bool hasHeightForWidth() const override {
        return true;
    }

    int heightForWidth(int width) const override;

protected:
    void  setupUi();
    int   getTitleHeight() const;
    int   getTitleOffset() const;
    QRect getItemRect(int row, int col) const;
    QRect getItemTitleRect(const QRect &item_rect) const;
    int   getItemIndex(const QPoint &pos) const;
    void  drawDisplayCase(QPainter *p, int index, int row, int col);

    void paintEvent(QPaintEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct DisplayCaseItem {
        QUrl    source;
        QPixmap cover;
        QString title;
    };

    QList<DisplayCaseItem> items_;
    int                    hover_index_;
    QSize                  item_size_;
    int                    item_spacing_;
};

} // namespace jwrite::Ui
