#include <jwrite/RwLock.h>

namespace jwrite::core {

RwLock::RwLock()
    : state_{0} {}

bool RwLock::try_lock_read() {
    int val = state_.load(std::memory_order_relaxed);
    if (val < 0) { return false; }
    return state_.compare_exchange_weak(val, val + 1);
}

void RwLock::lock_read() {
    while (!try_lock_read()) {}
}

void RwLock::unlock_read() {
    state_.fetch_sub(1, std::memory_order_relaxed);
}

bool RwLock::try_lock_write() {
    int val = state_.load(std::memory_order_relaxed);
    if (val != 0) { return false; }
    return state_.compare_exchange_weak(val, -1);
}

void RwLock::lock_write() {
    while (!try_lock_write()) {}
}

void RwLock::unlock_write() {
    state_.store(0, std::memory_order_relaxed);
}

bool RwLock::on_read() const {
    return state_.load(std::memory_order_relaxed) > 0;
}

bool RwLock::on_write() const {
    return state_.load(std::memory_order_relaxed) == -1;
}

} // namespace jwrite::core
