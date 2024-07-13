#pragma once

#include <jwrite/ui/PrimaryEditor.h>

namespace jwrite::ui {

class Jeditor : public PrimaryEditor {
    Q_OBJECT

public:
    explicit Jeditor(QWidget *parent = nullptr);

    bool soft_center_enabled() const;
    void set_soft_center_enabled(bool enabled);

    bool elastic_resize_enabled() const;
    void set_elastic_resize_enabled(bool enabled);

    int horizontal_margin_hint() const override;

    Tokenizer                *tokenizer() const override;
    AbstractTextRestrictRule *restrict_rule() const override;

    bool insert_action_filter(const QString &text, bool batch_mode) override;
    void internal_action_on_leave_block(int block_index) override;

    void execute_text_input_command(TextInputCommand cmd, const QString &text) override;

    void draw(QPainter *p) override;

private:
    bool try_exit_quotes();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    AbstractTextRestrictRule *restrict_rule_;
    Tokenizer                *tokenizer_;
    bool                      soft_center_;
    bool                      elastic_resize_;
};

} // namespace jwrite::ui
