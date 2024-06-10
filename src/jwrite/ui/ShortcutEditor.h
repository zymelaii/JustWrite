#pragma once

#include <QWidget>

namespace jwrite::ui {

class ShortcutEditor : public QWidget {
    Q_OBJECT

signals:
    void on_shortcut_change(QKeySequence shortcut);

public slots:
    void set_shortcut(const QKeySequence &shortcut);

protected:
    void    set_listen_enabled(bool enabled);
    QString display_text() const;

public:
    explicit ShortcutEditor(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void init();

    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QKeySequence shortcut_;

    bool ui_on_listen_;
    bool ui_hover_;
};

}; // namespace jwrite::ui
