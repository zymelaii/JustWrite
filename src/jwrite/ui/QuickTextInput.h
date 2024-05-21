#pragma once

#include <jwrite/BookManager.h>
#include <qt-material/qtmaterialtextfield.h>
#include <qt-material/qtmaterialraisedbutton.h>
#include <QWidget>
#include <QLabel>

namespace jwrite::ui {

class QuickTextInput : public QWidget {
    Q_OBJECT

public:
    explicit QuickTextInput(QWidget *parent = nullptr);
    ~QuickTextInput() override;

public:
    void    setText(const QString &text);
    QString text() const;
    void    setPlaceholderText(const QString &text);
    void    setLabel(const QString &label);

signals:
    void submitRequested(QString text);
    void cancelRequested();

protected:
    void setupUi();

    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QtMaterialTextField *ui_input_;
};

} // namespace jwrite::ui
