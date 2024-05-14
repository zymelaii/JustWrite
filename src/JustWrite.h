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

    void create_new_volume(int index, const QString &title);
    void create_new_chapter(int volume_index, const QString &title);
    void open_chapter(int cid);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    JustWritePrivate *d;
    Ui::JustWrite    *ui;
};
