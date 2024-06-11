#pragma once

#include <QLabel>

namespace widgetkit {

class ElidedLabel : public QLabel {
    Q_OBJECT

public:
    void set_elide_mode(Qt::TextElideMode elide_mode);

protected:
    void refresh_elided_text();

public:
    explicit ElidedLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit ElidedLabel(
        const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Qt::TextElideMode elide_mode_;
    QString           cached_elided_text_;
    QString           cached_text_;
};

} // namespace widgetkit
