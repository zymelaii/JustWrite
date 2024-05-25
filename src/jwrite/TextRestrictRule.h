#pragma once

#include <QString>
#include <functional>
#include <optional>

namespace jwrite {

class AbstractTextRestrictRule {
public:
    using RestrictSendFn = std::function<void(const QString& text, bool has_more)>;

public:
    virtual ~AbstractTextRestrictRule() = default;

    virtual QString restrict(const QString& text, QStringView before, QStringView after) const = 0;

    virtual void restrict_inplace(QString& text, QStringView before, QStringView after) const {
        text = restrict(text, before, after);
    }

    virtual void restrict_iter(
        const QString& text,
        QStringView    before,
        QStringView    after,
        RestrictSendFn send_fn) const = 0;
};

class TextRestrictRule : public AbstractTextRestrictRule {
protected:
    enum class Category {
        Null,
        Space,
        Normal,
        Digit,
        Alpha,
        LatinPunct,
        FullwidthPunct,
        Unknown,
    };

public:
    QString restrict(const QString& text, QStringView before, QStringView after) const override;

    void restrict_iter(
        const QString& text,
        QStringView    before,
        QStringView    after,
        RestrictSendFn send_fn) const override;

protected:
    static std::optional<QString>
        test_and_restrict_windowed_chars(QChar c_prev, QChar c_in, QChar c_next);

    static Category get_category(QChar c);
};

} // namespace jwrite
