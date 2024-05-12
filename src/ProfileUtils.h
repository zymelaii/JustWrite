#pragma once

#include <QList>
#include <QTimer>
#include <QDebug>
#include <chrono>
#include <array>
#include <magic_enum.hpp>

#ifndef NDEBUG
#define ON_DEBUG(...) __VA_ARGS__
#else
#define ON_DEBUG(...)
#endif

#ifndef NDEBUG
enum class ProfileTarget {
    IME2UpdateDelay,
    FrameRenderCost,
    TextEngineRenderCost,
    GeneralTextEdit,
    SwitchChapter,
};

struct Profiler : public QObject {
    using timestamp_t     = decltype(std::chrono::system_clock::now());
    using duration_t      = std::chrono::milliseconds;
    using duration_list_t = QList<duration_t>;
    using start_record_t  = std::array<timestamp_t, magic_enum::enum_count<ProfileTarget>()>;
    using profile_data_t  = std::array<duration_list_t, magic_enum::enum_count<ProfileTarget>()>;

    start_record_t start_record;
    profile_data_t profile_data;
    QTimer        *timer = nullptr;

    int indexof(ProfileTarget target) {
        return *magic_enum::enum_index(target);
    }

    void start(ProfileTarget target) {
        auto &rec = start_record[indexof(target)];
        if (rec.time_since_epoch().count() == 0) { rec = std::chrono::system_clock::now(); }
    }

    void record(ProfileTarget target) {
        const auto index = indexof(target);
        auto      &rec   = start_record[index];
        if (rec.time_since_epoch().count() != 0) {
            auto now = std::chrono::system_clock::now();
            profile_data[index].push_back(std::chrono::duration_cast<duration_t>(now - rec));
            rec = timestamp_t{};
        }
    }

    int total_valid() {
        int count = 0;
        for (auto data : profile_data) {
            if (!data.empty()) { ++count; }
        }
        return count;
    }

    float averageof(ProfileTarget target) {
        const auto &data = profile_data[indexof(target)];
        float       sum  = 0;
        for (auto dur : data) { sum += dur.count(); }
        return sum / data.size();
    }

    void clear(ProfileTarget target) {
        profile_data[indexof(target)].clear();
    }

    void setup() {
        timer = new QTimer(this);
        timer->setInterval(10000);
        timer->setSingleShot(false);
        timer->start();
        QObject::connect(timer, &QTimer::timeout, this, [this]() {
            if (total_valid() == 0) { return; }
            qDebug().noquote() << QStringLiteral("PROFILE DATA");
            for (auto target : magic_enum::enum_values<ProfileTarget>()) {
                const auto &data = profile_data[indexof(target)];
                if (data.empty()) { continue; }
                qDebug().noquote() << QStringLiteral("  %1 %2ms")
                                          .arg(magic_enum::enum_name(target).data())
                                          .arg(averageof(target));
                clear(target);
            }
        });
    }
};

extern Profiler JwriteProfiler;
#endif
