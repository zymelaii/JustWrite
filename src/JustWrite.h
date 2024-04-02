#pragma once

#include <QWidget>

namespace Ui {
struct JustWrite;
}; // namespace Ui

struct JustWritePrivate;

class JustWrite : public QWidget {
    Q_OBJECT

public:
    explicit JustWrite(QWidget *parent = nullptr);
    virtual ~JustWrite();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    JustWritePrivate *d;
    Ui::JustWrite    *ui;
};
