#pragma once

#include <boost/noncopyable.hpp>
#include <fmt/format.h>
#include <mutex>

namespace trade::utilities
{

/// SnowFlaker implements a snowflake ID generator, a form of unique identifier
/// used in distributed computing.
/// See https://en.wikipedia.org/wiki/Snowflake_ID and
/// https://blog.twitter.com/engineering/en_us/a/2010/announcing-snowflake.
template<int64_t TEpoch, typename Lock = std::mutex>
class SnowFlaker: private boost::noncopyable
{
public:
    SnowFlaker() = default;

    void init(const int64_t worker_id, const int64_t datacenter_id)
    {
        if (worker_id > MAX_WORKER_ID || worker_id < 0) {
            throw std::runtime_error(fmt::format("Worker ID should be between 0 and {}", MAX_DATACENTER_ID));
        }

        if (datacenter_id > MAX_DATACENTER_ID || datacenter_id < 0) {
            throw std::runtime_error(fmt::format("Datacenter ID should be between 0 and {}", MAX_DATACENTER_ID));
        }

        m_worker_id     = worker_id;
        m_datacenter_id = datacenter_id;
    }

    [[nodiscard]] int64_t next()
    {
        std::lock_guard lock(m_lock);

        auto timestamp = millisecond();
        if (m_last_timestamp == timestamp) {
            m_sequence = m_sequence + 1 & SEQUENCE_MASK;
            if (m_sequence == 0) {
                timestamp = waitForNextMillis(m_last_timestamp);
            }
        }
        else {
            m_sequence = 0;
        }

        m_last_timestamp = timestamp;

        return timestamp - TWEPOCH << TIMESTAMP_LEFT_SHIFT
             | m_datacenter_id << DATACENTER_ID_SHIFT
             | m_worker_id << WORKER_ID_SHIFT
             | m_sequence;
    }

    [[nodiscard]] int64_t operator()()
    {
        return next();
    }

private:
    [[nodiscard]] int64_t millisecond() const noexcept
    {
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start_time_point);
        return m_start_millisecond + diff.count();
    }

    [[nodiscard]] int64_t waitForNextMillis(const int64_t last) const noexcept
    {
        auto timestamp = millisecond();
        while (timestamp <= last) {
            timestamp = millisecond();
        }
        return timestamp;
    }

private:
    using lock_type                               = Lock;

    static constexpr int64_t TWEPOCH              = TEpoch;
    static constexpr int64_t WORKER_ID_BITS       = 5L;
    static constexpr int64_t DATACENTER_ID_BITS   = 5L;
    static constexpr int64_t MAX_WORKER_ID        = (1 << WORKER_ID_BITS) - 1;
    static constexpr int64_t MAX_DATACENTER_ID    = (1 << DATACENTER_ID_BITS) - 1;
    static constexpr int64_t SEQUENCE_BITS        = 12L;
    static constexpr int64_t WORKER_ID_SHIFT      = SEQUENCE_BITS;
    static constexpr int64_t DATACENTER_ID_SHIFT  = SEQUENCE_BITS + WORKER_ID_BITS;
    static constexpr int64_t TIMESTAMP_LEFT_SHIFT = SEQUENCE_BITS + WORKER_ID_BITS + DATACENTER_ID_BITS;
    static constexpr int64_t SEQUENCE_MASK        = (1 << SEQUENCE_BITS) - 1;

    using SteadyClockType                            = std::chrono::time_point<std::chrono::steady_clock>;
    SteadyClockType m_start_time_point               = std::chrono::steady_clock::now();

    int64_t m_start_millisecond                   = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t m_last_timestamp                      = -1;
    int64_t m_worker_id                           = 0;
    int64_t m_datacenter_id                       = 0;
    int64_t m_sequence                            = 0;
    lock_type m_lock;
};

/// Singleton class.
class SnowFlakerG
{
private:
    SnowFlakerG()
    {
        m_sf.init(1, 1);
    }

public:
    [[nodiscard]] static SnowFlakerG& get_generator()
    {
        static SnowFlakerG generator;
        return generator;
    }

    [[nodiscard]] int64_t operator()()
    {
        return m_sf();
    }

private:
    /// Start from 2000-01-01 00:00:00 GMT.
    SnowFlaker<946684800000l> m_sf;
};

/// Get a new order ID.
#define NEW_ID trade::utilities::SnowFlakerG::get_generator()

/// Get a new order ID as a string (only numeric).
#define NEW_ID_S() std::to_string(NEW_ID())

/// Invalid order ID.
#define INVALID_ID (0)

} // namespace trade::utilities
