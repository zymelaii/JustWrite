#pragma once

#include <atomic>

namespace jwrite::core {

class RwLock {
public:
    RwLock();
    RwLock(const RwLock &)            = delete;
    RwLock &operator=(const RwLock &) = delete;

    bool try_lock_read();
    void lock_read();
    void unlock_read();

    bool try_lock_write();
    void lock_write();
    void unlock_write();

    bool on_read() const;
    bool on_write() const;

    template <typename T, typename G>
    auto lock_guard(T &&on_ctor, G &&on_dtor) {
        class LockGuard {
        public:
            LockGuard(RwLock *lock, T &&on_ctor, G &&on_dtor)
                : lock_{lock}
                , on_ctor_{on_ctor}
                , on_dtor_{on_dtor} {
                std::invoke(on_ctor_, lock_);
            }

            ~LockGuard() {
                std::invoke(on_dtor_, lock_);
            }

        private:
            RwLock *lock_;
            T       on_ctor_;
            G       on_dtor_;
        };

        return LockGuard(this, std::forward<T>(on_ctor), std::forward<G>(on_dtor));
    }

    auto read_lock_guard() {
        return lock_guard(&RwLock::lock_read, &RwLock::unlock_read);
    }

    auto write_lock_guard() {
        return lock_guard(&RwLock::lock_write, &RwLock::unlock_write);
    }

private:
    std::atomic_int state_;
};

} // namespace jwrite::core
