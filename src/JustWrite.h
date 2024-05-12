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

    void newVolume(int index, const QString &title);
    void newChapter(int volume_index, const QString &title);
    void openChapter(int cid);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    JustWritePrivate *d;
    Ui::JustWrite    *ui;
};
