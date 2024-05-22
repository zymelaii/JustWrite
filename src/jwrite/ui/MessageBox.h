#pragma once

#include <jwrite/ui/FlatButton.h>
#include <jwrite/ui/Label.h>
#include <QWidget>
#include <QLabel>
#include <QEventLoop>

namespace jwrite::ui {

class MessageBox : public QWidget {
    Q_OBJECT

public:
    enum Choice {
        Yes,
        No,
        Cancel,
    };

public:
    explicit MessageBox(QWidget *parent = nullptr);
    ~MessageBox();

signals:
    void choiceRequested(Choice choice);

public:
    void setCaption(const QString &caption) {
        ui_caption_->setText(caption);
    }

    void setText(const QString &message) {
        ui_message_->setText(message);
    }

    int exec();

protected:
    void setupUi();
    void setupConnections();
    void notifyChoice(Choice choice);

    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLabel     *ui_caption_;
    Label      *ui_close_;
    QLabel     *ui_icon_;
    QLabel     *ui_message_;
    FlatButton *ui_btn_no_;
    FlatButton *ui_btn_yes_;
    QEventLoop *event_loop_;
};

} // namespace jwrite::ui
