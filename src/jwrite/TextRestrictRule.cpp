#include <jwrite/TextRestrictRule.h>

namespace jwrite {

QString
    TextRestrictRule::restrict(const QString &text, QStringView before, QStringView after) const {
    if (text.isEmpty()) { return text; }

    QString result{};

    const QChar last_bound_char = after.isEmpty() ? QChar::Null : after.front();

    QChar char_window[3];
    char_window[0] = before.isEmpty() ? QChar::Null : before.back();
    char_window[1] = text.front();
    char_window[2] = text.length() > 1 ? text.at(1) : last_bound_char;

    auto &c_prev = char_window[0];
    auto &c_in   = char_window[1];
    auto &c_next = char_window[2];

    for (int i = 0; i + 1 < text.length(); ++i) {
        if (auto opt = test_and_restrict_windowed_chars(c_prev, c_in, c_next)) {
            result.append(*opt);
        }

        c_prev = result.isEmpty() ? QChar::Null : result.back();
        c_in   = text.at(i + 1);
        c_next = i + 2 < text.length() ? text.at(i + 2) : last_bound_char;
    }

    if (text.length() > 1) {
        c_prev = result.isEmpty() ? QChar::Null : result.back();
        c_in   = text.back();
        c_next = last_bound_char;
    }

    if (auto opt = test_and_restrict_windowed_chars(c_prev, c_in, c_next)) { result.append(*opt); }

    return result;
}

void TextRestrictRule::restrict_iter(
    const QString &text, QStringView before, QStringView after, RestrictSendFn send_fn) const {
    //! TODO: to be implemented
    return;
}

std::optional<QString>
    TextRestrictRule::test_and_restrict_windowed_chars(QChar c_prev, QChar c_in, QChar c_next) {
    const auto c1 = get_category(c_prev);
    const auto c2 = get_category(c_in);
    const auto c3 = get_category(c_next);

    if (c2 == Category::Space) {
        if ((c1 == Category::Digit || c1 == Category::Alpha)
            && (c3 == Category::Digit || c3 == Category::Alpha || c3 == Category::Null
                || c3 == Category::Normal)) {
            return {QChar(QChar::Space)};
        }
        if (c1 == Category::LatinPunct && c3 != Category::LatinPunct) {
            return {QChar(QChar::Space)};
        }
        return std::nullopt;
    }

    if (c2 == Category::Digit || c2 == Category::Alpha) {
        QString result{c_in};
        if (c1 == Category::LatinPunct || c1 == Category::Normal) {
            result.prepend(QChar(QChar::Space));
        }
        //! FIXME: this would break continous digits/alphas input
        // if (c3 == Category::Normal) { result.append(QChar(QChar::Space)); }
        return result;
    }

    if (c2 == Category::Normal) {
        QString result{c_in};
        if (c1 == Category::LatinPunct || c1 == Category::Digit || c1 == Category::Alpha) {
            result.prepend(QChar(QChar::Space));
        }
        if (c3 == Category::Digit || c3 == Category::Alpha) { result.append(QChar(QChar::Space)); }
        return result;
    }

    if (c2 == Category::LatinPunct) {
        QString result{c_in};
        if (c3 != Category::Null && c3 != Category::Space && c3 != Category::LatinPunct
            && c3 != Category::FullwidthPunct) {
            result.append(QChar(QChar::Space));
        }
        return result;
    }

    if (c2 == Category::FullwidthPunct) { return {c_in}; }

    return std::nullopt;
}

TextRestrictRule::Category TextRestrictRule::get_category(QChar c) {
    if (c.isSpace()) { return Category::Space; }
    if (c.isNull()) { return Category::Null; }
    if (c.isDigit() && c.unicode() < 0x7f) { return Category::Digit; }
    if (c.isUpper() || c.isLower() || c.isTitleCase()) { return Category::Alpha; }
    if (c.isLetter() || c.isSymbol()) { return Category::Normal; }
    if (c.isPunct()) {
        if (c.unicode() > 0x7f) { return Category::FullwidthPunct; }
        if (c == ',' || c == '?' || c == '!' || c == '.') { return Category::LatinPunct; }
        return Category::Alpha;
    }
    return Category::Unknown;
}

} // namespace jwrite
