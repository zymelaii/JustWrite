#pragma once

#include <jwrite/ui/SystemButton.h>
#include <jwrite/ColorScheme.h>
#include <QWidget>
#include <magic_enum.hpp>
#include <array>

namespace jwrite::ui {

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    virtual ~TitleBar();

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

public:
    QString title() const;
    void    setTitle(const QString &title);

    SystemButton *button(SystemButton::SystemCommand command_type) const;

    SystemButton *close_button() const {
        return button(SystemButton::Close);
    }

    SystemButton *minimize_button() const {
        return button(SystemButton::Minimize);
    }

    SystemButton *maximize_button() const {
        return button(SystemButton::Maximize);
    }

    void updateColorScheme(const ColorScheme &scheme);

protected slots:
    void requestMinimize();
    void requestMaximize();
    void requestClose();

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();
    void setupConnections();

    void paintEvent(QPaintEvent *event) override;

private:
    using SystemButtonGroup =
        std::array<SystemButton *, magic_enum::enum_count<SystemButton::SystemCommand>()>;

    QString           title_;
    SystemButtonGroup sys_buttons_;
};

} // namespace jwrite::ui
