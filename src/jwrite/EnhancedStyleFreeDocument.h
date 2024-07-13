#pragma once

#include <QPainter>

namespace jwrite::core {

class EnhancedStyleFreeDocument;

class DocumentBlock {
public:
    DocumentBlock(EnhancedStyleFreeDocument* doc)
        : document_{doc} {
        Q_ASSERT(document_ != nullptr);
    }

    virtual ~DocumentBlock() = default;

    EnhancedStyleFreeDocument* document() const {
        return document_;
    }

    void set_text(const QString& text) {
        text_ = text;
    }

    QString text() const {
        return text_;
    }

    virtual int  height() const                     = 0;
    virtual void draw(QPainter& p, const QRect& bb) = 0;

private:
    EnhancedStyleFreeDocument* document_;
    QString                    text_;
};

class TextBlock : public DocumentBlock {};

class QuoteBlock : public DocumentBlock {};

class SubTitleBlock : public DocumentBlock {};

class TitleBlock : public DocumentBlock {};

class SeparatorBlock : public DocumentBlock {};

class IllustrationBlock : public DocumentBlock {};

} // namespace jwrite::core
