#include <jwrite/WordCounter.h>
#include <QSet>

namespace jwrite {

int LossenWordCounter::count_all(const QString& text) {
    int count = text.length();
    for (const auto c : text) {
        if (c.isSpace()) { --count; }
    }
    return count;
}

int StrictWordCounter::count_all(const QString& text) {
    int count = 0;

    State next_state = State::Unknown;
    State state      = next_state;

    for (const auto ch : text) {
        next_state = predicate_char_state(ch);
        switch (state) {
            case State::Unknown:
                [[fallthrough]];
            case State::Blank:
                [[fallthrough]];
            case State::Punctuation: {
            } break;
            case State::Zero: {
                if (ch == 'b' || ch == 'B' || ch == 'o' || ch == 'x' || ch == 'X'
                    || next_state == State::Zero) {
                    next_state = State::NumberPrefix;
                } else if (ch == '.' || next_state == State::Number) {
                    next_state = State::Number;
                } else {
                    ++count;
                }
            } break;
            case State::NumberPrefix: {
                if (next_state == State::Zero) { next_state = State::Number; }
            } break;
            case State::Number: {
                if (next_state == State::Number || next_state == State::Zero || ch == '.') {
                    next_state = State::Number;
                } else {
                    ++count;
                }
            } break;
            case State::Character: {
                ++count;
            } break;
            case State::Word: {
                if (next_state == State::Word || ch == '-') {
                    next_state = State::Word;
                } else {
                    ++count;
                }
            } break;
        }
        state = next_state;
    }

    if (state != State::Unknown && state != State::Blank && state != State::Punctuation) {
        ++count;
    }

    return count;
}

StrictWordCounter::State StrictWordCounter::predicate_char_state(QChar ch) {
    if (ch.isSpace()) { return State::Blank; }
    if (ch.isTitleCase() || ch.isUpper() || ch.isLower()) { return State::Word; }
    if (ch == '0') { return State::Zero; }
    if (ch.isDigit()) { return State::Number; }
    if (ch.isPunct()) { return State::Punctuation; }
    if (ch.isLetter()) { return State::Character; }
    return State::Unknown;
}

} // namespace jwrite
