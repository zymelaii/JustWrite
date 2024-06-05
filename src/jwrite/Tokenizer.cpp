#include <jwrite/Tokenizer.h>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <vector>
#include <string>

namespace jwrite {

QFuture<Tokenizer *> Tokenizer::build() {
    return QtConcurrent::run([] {
        const auto dir      = QCoreApplication::applicationDirPath() + "/dicts";
        auto       instance = new Tokenizer;
        instance->cutter_   = std::make_unique<cppjieba::Jieba>(
            (dir + "/jieba.dict.utf8").toLocal8Bit().toStdString(),
            (dir + "/hmm_model.utf8").toLocal8Bit().toStdString(),
            (dir + "/user.dict.utf8").toLocal8Bit().toStdString(),
            (dir + "/idf.utf8").toLocal8Bit().toStdString(),
            (dir + "/stop_words.utf8").toLocal8Bit().toStdString());
        return instance;
    });
}

Tokenizer &Tokenizer::get_instance() {
    static std::unique_ptr<Tokenizer> instance{};
    if (!instance) { instance.reset(Tokenizer::build().result()); }
    return *instance;
}

QStringList Tokenizer::cut(const QString &sentence) const {
    if (sentence.isEmpty()) { return {}; }
    std::vector<std::string> words{};
    cutter_->Cut(sentence.toStdString(), words, true);
    QStringList results{};
    for (const auto &word : words) { results << QString::fromStdString(word); }
    return results;
}

QString Tokenizer::get_last_word(const QString &sentence) const {
    if (sentence.isEmpty()) { return ""; }
    const int                max_len = 16;
    std::vector<std::string> words{};
    cutter_->Cut(sentence.right(max_len).toStdString(), words, true);
    return QString::fromStdString(words.back());
}

QString Tokenizer::get_first_word(const QString &sentence) const {
    if (sentence.isEmpty()) { return ""; }
    const int                max_len = 16;
    std::vector<std::string> words{};
    cutter_->Cut(sentence.left(max_len).toStdString(), words, true);
    return QString::fromStdString(words.front());
}

} // namespace jwrite
