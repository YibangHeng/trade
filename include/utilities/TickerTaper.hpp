#pragma once

#include <atomic>

namespace trade::utilities
{

template<typename T>
class TickerTaper
{
public:
    TickerTaper() : m_seq_id(0) {}
    explicit TickerTaper(T value) : m_seq_id(value) {}
    ~TickerTaper() = default;

public:
    /// Returns the next value in the sequence.
    /// @return The next value in the sequence.
    T operator()()
    {
        return m_seq_id.fetch_add(1);
    }

    /// Resets the value to 0.
    void reset()
    {
        m_seq_id.store(0);
    }

private:
    std::atomic<T> m_seq_id;
};

/// Specialization for std::string.
template<>
class TickerTaper<std::string>
{
public:
    TickerTaper() : m_seq_id(0) {}
    explicit TickerTaper(const int64_t value) : m_seq_id(value) {}
    ~TickerTaper() = default;

public:
    /// Returns the next value in the sequence.
    /// @return The next value in the sequence.
    std::string operator()()
    {
        return std::to_string(m_seq_id.fetch_add(1));
    }

    /// Resets the value to 0.
    void reset()
    {
        m_seq_id.store(0);
    }

private:
    std::atomic<int64_t> m_seq_id;
};

} // namespace trade::utilities
