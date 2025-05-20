#pragma once

#include <bsl/log.hpp>
#include <chrono>

DEFINE_LOG_CATEGORY(Perf)

class ScopedTimer {
public:
    ScopedTimer(const std::string& category, const std::string &name)
        : m_Category(category), m_Name(name), m_StartTimePoint(std::chrono::high_resolution_clock::now())
    {
    }

    ~ScopedTimer() {
        Stop();
    }

private:
    void Stop() {
        auto endTimePoint = std::chrono::high_resolution_clock::now();
        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();

        auto duration = end - start;
        double ms = duration * 0.001;
        
        LogPerf(Display, "[%] % took % us(% ms)", m_Category, m_Name, duration, ms);
    }

    std::string m_Name;
    std::string m_Category;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;
};

#if WITH_PROFILE
#define PROFILE_SCOPE(category, name) ScopedTimer __timer##category##name##__LINE__(#category, #name)
#else
#define PROFILE_SCOPE(category, name) ((void)0)
#endif
