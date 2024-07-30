#pragma once

namespace trade::utilities
{

template<typename T>
class SeqChecker
{
public:
    explicit SeqChecker(T start_seq = 0)
        : m_last_seq(start_seq)
    {
    }
    ~SeqChecker() = default;

public:
    T diff(T seq)
    {
        auto diff  = seq - m_last_seq;
        m_last_seq = seq;
        return diff;
    }

private:
    T m_last_seq;
};

} // namespace trade::utilities