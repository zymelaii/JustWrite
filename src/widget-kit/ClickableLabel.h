#pragma once

#include <QLabel>

namespace widgetkit {

class ClickableLabel : public QLabel {
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit ClickableLabel(
        const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~ClickableLabel() override;

signals:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

} // namespace widgetkit
