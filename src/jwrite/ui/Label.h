#pragma once

#include <QLabel>

namespace jwrite::ui {

class Label : public QLabel {
    Q_OBJECT

public:
    explicit Label(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit Label(
        const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    ~Label() override;

signals:
    void pressed();

protected:
    void mousePressEvent(QMouseEvent *QMouseEvent) override;
};

} // namespace jwrite::ui
