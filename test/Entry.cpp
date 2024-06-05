#include <gtest/gtest.h>
#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[]) {
    QApplication headless(argc, argv);

    const auto font_name = u8"SarasaGothicSC-Light";
    QFontDatabase::addApplicationFont(QString("fonts/%1.ttf").arg(font_name));
    QApplication::setFont(QFont(font_name, 16));

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
