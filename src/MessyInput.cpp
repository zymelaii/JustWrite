#include "MessyInput.h"
#include <random>

#ifdef WIN32
#include <windows.h>
#endif

void MessyInputWorker::start() {
    enabled_ = true;
    QThread::start();
}

void MessyInputWorker::kill() {
    if (!enabled_) { return; }
    enabled_ = false;
    terminate();
    wait();
}

void MessyInputWorker::send_ime_key(int key) {
#ifdef WIN32
    if (key == Qt::Key_Return) { key = VK_RETURN; }
    keybd_event(key, 0, 0, 0);
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
#endif
}

void MessyInputWorker::run() {
    std::random_device{};
    std::mt19937                       rng{std::random_device{}()};
    std::uniform_int_distribution<int> type_dist{1, 12};
    std::uniform_int_distribution<int> char_dist{'A', 'Z'};
    std::uniform_int_distribution<int> nl_dist{0, 4};

    while (enabled_) {
        const int type_times = type_dist(rng);
        for (int j = 0; enabled_ && j < type_times; ++j) {
            const auto ch = char_dist(rng);
            send_ime_key(ch);
            msleep(1);
        }

        if (!enabled_) { break; }

        send_ime_key(Qt::Key_Space);
        msleep(10);

        if (!enabled_) { break; }

        if (nl_dist(rng) == 0) {
            send_ime_key(Qt::Key_Return);
            msleep(10);
        }
    }
}
