#pragma once

#include <QString>

namespace jwrite {

class AbstractWordCounter {
public:
    virtual ~AbstractWordCounter() = default;

    virtual int count_all(const QString& text) = 0;

    virtual int count_and_cache(int key, const QString& text) {
        return count_all(text);
    }
};

class LossenWordCounter : public AbstractWordCounter {
public:
    int count_all(const QString& text) override;
};

class StrictWordCounter : public AbstractWordCounter {
public:
    enum class State {
        Unknown,
        Blank,
        Punctuation,
        Zero,
        NumberPrefix,
        Number,
        Character,
        Word, //<! refers to english word
    };

public:
    int count_all(const QString& text) override;

    static State predicate_char_state(QChar ch);
};

} // namespace jwrite
