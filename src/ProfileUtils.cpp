#ifndef NDEBUG

#include "ProfileUtils.h"
#include <QDebug>

void Profiler::setup() {
    timer_ = new QTimer(this);
    timer_->setInterval(30000);
    timer_->setSingleShot(false);
    timer_->start();
    QObject::connect(timer_, &QTimer::timeout, this, [this]() {
        if (total_valid() == 0) { return; }
        qDebug().noquote() << QStringLiteral("PROFILE DATA");
        for (auto target : magic_enum::enum_values<ProfileTarget>()) {
            const auto &data = profile_data_[indexof(target)];
            if (data.empty()) { continue; }
            qDebug().noquote() << QStringLiteral("  %1 %2us")
                                      .arg(magic_enum::enum_name(target).data())
                                      .arg(averageof(target));
            clear(target);
        }
    });
}

void Profiler::start(ProfileTarget target) {
    auto &rec = start_record_[indexof(target)];
    if (rec.time_since_epoch().count() == 0) { rec = std::chrono::system_clock::now(); }
}

void Profiler::record(ProfileTarget target) {
    const auto index = indexof(target);
    auto      &rec   = start_record_[index];
    if (rec.time_since_epoch().count() != 0) {
        auto now = std::chrono::system_clock::now();
        profile_data_[index].push_back(std::chrono::duration_cast<duration_t>(now - rec));
        rec = timestamp_t{};
    }
}

int Profiler::total_valid() const {
    int count = 0;
    for (auto data : profile_data_) {
        if (!data.empty()) { ++count; }
    }
    return count;
}

int Profiler::indexof(ProfileTarget target) const {
    return *magic_enum::enum_index(target);
}

float Profiler::averageof(ProfileTarget target) const {
    const auto &data = profile_data_[indexof(target)];
    float       sum  = 0;
    for (auto dur : data) { sum += dur.count(); }
    return sum / data.size();
}

void Profiler::clear(ProfileTarget target) {
    profile_data_[indexof(target)].clear();
}

Profiler JwriteProfiler;

#endif
