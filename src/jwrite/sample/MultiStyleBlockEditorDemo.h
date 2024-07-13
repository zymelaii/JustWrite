#pragma once

#include <QWidget>

namespace jwrite::sample {

class EnhancedStyleFreeDocument;

class AbstractBlock {
public:
    virtual void draw(QPainter *p, const QRect &bb) = 0;
    virtual int  height() const                     = 0;
};

class TextBlock : public AbstractBlock {
public:
    void draw(QPainter *p, const QRect &bb) override {}

    int height() const override {
        return 100;
    }

    void set_font(const QFont &font) {
        font_ = font;
    }

    void set_text(const QString &text) {}

private:
    QFont font_;
};

class MultiStyleBlockEditorDemo : public QWidget {
    Q_OBJECT

public:
};

} // namespace jwrite::sample
