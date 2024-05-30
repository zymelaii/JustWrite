#pragma once

#include <cppjieba/Jieba.hpp>
#include <QtCore>
#include <memory>

namespace jwrite {

class Tokenizer {
public:
    static Tokenizer *build();
    static Tokenizer &get_instance();

    QStringList cut(const QString &sentence) const;
    QString     get_last_word(const QString &sentence) const;
    QString     get_first_word(const QString &sentence) const;

private:
    std::unique_ptr<cppjieba::Jieba> cutter_;
};

} // namespace jwrite
