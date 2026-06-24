#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace pypilot_runtime {

struct RuntimeWatchPeriod {
    char name[80]{};
    bool continuous = true;
    bool pending = false;
    uint64_t period_us = 0;
    uint64_t next_due_us = 0;
};

static inline uint64_t runtime_watch_period_to_us(double seconds) {
    if (seconds <= 0.0) {
        return 0;
    }
    return static_cast<uint64_t>(seconds * 1000000.0);
}

static inline RuntimeWatchPeriod make_runtime_watch_period(const char* name,
                                                           double seconds,
                                                           uint64_t now_us) {
    RuntimeWatchPeriod watch;
    if (name) {
        strncpy(watch.name, name, sizeof(watch.name) - 1);
        watch.name[sizeof(watch.name) - 1] = '\0';
    }
    watch.period_us = runtime_watch_period_to_us(seconds);
    watch.continuous = watch.period_us == 0;
    watch.pending = false;
    watch.next_due_us = now_us + watch.period_us;
    return watch;
}

static inline bool mark_runtime_watch_pending(RuntimeWatchPeriod& watch, uint64_t now_us) {
    if (watch.continuous) {
        return true;
    }
    watch.pending = true;
    if (watch.next_due_us == 0 || watch.next_due_us < now_us) {
        watch.next_due_us = now_us;
    }
    return false;
}

static inline bool runtime_watch_due(RuntimeWatchPeriod& watch, uint64_t now_us) {
    if (watch.continuous) {
        return true;
    }
    if (!watch.pending || now_us < watch.next_due_us) {
        return false;
    }
    watch.pending = false;
    watch.next_due_us = now_us + watch.period_us;
    return true;
}

} // namespace pypilot_runtime
