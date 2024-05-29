#pragma once

#include <widget-kit/ClickableLabel.h>

namespace jwrite::ui {

class ToolbarIcon : public widgetkit::ClickableLabel {
    Q_OBJECT

public:
    int  get_icon_size() const;
    void set_icon_size(int size);
    void set_icon(const QString& icon_name);
    void reload_icon();
    void set_action(QAction* action);
    void reset_action();
    void set_tool_tip(const QString& tip);

public slots:
    void trigger();

public:
    explicit ToolbarIcon(const QString& icon_name, QWidget* parent = nullptr);
    ~ToolbarIcon() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    bool event(QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QAction* action_;
    QString  icon_name_;
    int      icon_size_;
    QPixmap  ui_icon_;
    bool     ui_hover_;
};

} // namespace jwrite::ui
