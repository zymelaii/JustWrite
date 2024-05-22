#pragma once

#include <QWidget>

namespace jwrite::ui {

class PlainImageView : public QWidget {
    Q_OBJECT

public:
    explicit PlainImageView(QWidget *parent = nullptr);
    ~PlainImageView();

public:
    void setImage(const QString &path);
    void setViewSize(const QSize &size);
    void setBorderVisible(bool visible);

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();
    void reloadImage();

    void paintEvent(QPaintEvent *event) override;

public:
    bool    border_visible_;
    QSize   view_size_;
    QPixmap cached_pixmap_;
    QString image_path_;
};

} // namespace jwrite::ui
