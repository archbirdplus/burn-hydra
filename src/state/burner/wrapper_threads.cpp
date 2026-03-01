#include "kernels.h"
#include "communicate.h"
#include <iostream>
#include <cassert>
#include <thread>

thread_break::thread_break(uint64_t index) {
    right_high = index;
    undercarry = locked_fmpz();
    overcarry = locked_fmpz();
}

thread_break::thread_break(thread_break&& other) noexcept {
    right_high = other.right_high;
    std::swap(undercarry, other.undercarry);
    std::swap(overcarry, other.overcarry);
}

thread_break& thread_break::operator =(thread_break&& other) noexcept {
    if (this != &other) {
        right_high = other.right_high;
        std::swap(undercarry, other.undercarry);
        std::swap(overcarry, other.overcarry);
    }
    return *this;
}

Wrapper_threads::Wrapper_threads(Context* global_ctx, Wrapper* wrapper) {
    global_context = global_ctx;
    subscription = wrapper->add_subscriber(this);

    thread_scales = subscription.scales;
    next_scale = subscription.next_scale;
    uint64_t this_low = 0;
    auto breaks = global_context->task->thread_breaks[global_context->task->world_rank];
    thread_count = breaks.size() + 1;
    for (uint64_t i = 0; i <= breaks.size(); i++) {
        uint64_t this_high = i == breaks.size() ? thread_scales.size()-1 : breaks[i];
        if (i < breaks.size()) {
            thread_breaks.push_back(thread_break(this_high));
        }
        thread_sections.push_back({
            .high = this_high,
            .low = this_low,
            .id = i
        });
        this_low = this_high + 1;
    }
    vec<uint64_t> thread_starts = {};
    thread_starts.insert(thread_starts.end(), breaks.begin(), breaks.end());
    thread_starts.push_back(subscription.scales.size()-1);
}

Wrapper_threads::~Wrapper_threads() {
    
}

subscription_t Wrapper_threads::add_subscriber(Runnable* burner) {
    uint64_t max_burners = thread_sections.size();
    if (thread_burners.size() >= max_burners) {
        throw std::runtime_error("Added too many burners to a threads wrapper");
    }
    uint64_t index = thread_burners.size();
    thread_burners.push_back(burner);
    auto section = thread_sections[index];
    return {
        .scales = std::vector(thread_scales.begin()+section.low, thread_scales.begin()+section.high),
        .next_scale = section.low > 0 ? thread_scales[section.low] : next_scale,
        .can_push_right = index > 0 ? true : subscription.can_push_right,
        .can_push_left = index >= max_burners-1 ? subscription.can_push_left : true,
        .id = index
    };
}

void Wrapper_threads::run_thread(thread_section_t section, uint64_t end) {
    std::cout << "section " << section.id << " of burners " << thread_burners.size() << std::endl;
    thread_burners[section.id]->run_until(end);
}

void Wrapper_threads::run_until(uint64_t end) {
    vec<std::thread> threads = {};
    for (thread_section_t section : thread_sections) {
        threads.push_back(std::thread(&Wrapper_threads::run_thread, this, section, end));
    }
    for (std::thread &thread : threads) {
        thread.join();
    }
}

void Wrapper_threads::pullR(uint64_t id, timed_fmpz* x_import) {
    // TODO
}

void Wrapper_threads::pushR(uint64_t id, timed_fmpz* x_export) {
    // TODO
}

void Wrapper_threads::pushL(uint64_t id, timed_fmpz* x_export) {
    // TODO
}

void Wrapper_threads::pullL(uint64_t id, timed_fmpz* x_import) {
    // TODO
}



