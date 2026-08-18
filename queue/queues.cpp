#pragma once
#include <atomic>
#include <cstdint>

template <typename T, uint16_t N>
class SPSCQueue
{
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    alignas(64) T buffer_[N];
    alignas(64) std::atomic<uint16_t> head_;
    alignas(64) std::atomic<uint16_t> tail_;
    alignas(64) uint16_t cachedHead_{0};
    alignas(64) uint16_t cachedTail_{0};

public:
    SPSCQueue() : head_(0), tail_(0) {}

    bool push(const T& item)
    {
        uint16_t t = tail_.load(std::memory_order_relaxed);
        uint16_t nextTail = (t + 1) & (N - 1);
        if (nextTail == cachedHead_)
        {
            cachedHead_ = head_.load(std::memory_order_acquire);
            if (nextTail == cachedHead_)
                return false;
        }
        buffer_[t] = item;
        tail_.store(nextTail, std::memory_order_release);
        return true;
    }

    bool pop(T& item)
    {
        uint16_t h = head_.load(std::memory_order_relaxed);
        if (h == cachedTail_)
        {
            cachedTail_ = tail_.load(std::memory_order_acquire);
            if (h == cachedTail_)
                return false;
        }
        item = buffer_[h];
        head_.store((h + 1) & (N - 1), std::memory_order_release);
        return true;
    }
};