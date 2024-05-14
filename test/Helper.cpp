#include "Helper.h"
#include <QRandomGenerator>

int gen_random_nint(int lo, int hi) {
    return QRandomGenerator::global()->bounded(lo, hi);
}

QString gen_random(int length) {
    static const QString char_set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    QString              str;
    for (int i = 0; i < length; ++i) {
        int   index    = QRandomGenerator::global()->bounded(char_set.length());
        QChar nextChar = char_set.at(index);
        str.append(nextChar);
    }
    return str;
}
