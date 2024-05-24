#pragma once

#include <QPushButton>

namespace jwrite::ui {

class SystemButton : public QPushButton {
    Q_OBJECT

public:
    enum SystemCommand {
        Minimize,
        Maximize,
        Close,
    };

public:
    explicit SystemButton(SystemCommand command_type, QWidget* parent = nullptr);

protected:
    void setupUi();
    void reloadIcon();

    void paintEvent(QPaintEvent* event) override;

private:
    const SystemCommand cmd_;
    QColor              background_color_;
};

} // namespace jwrite::ui