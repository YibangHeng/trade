#pragma once

#include <cstring>

namespace trade::utilities
{

/// Make a char array assignable with = and +=.
/// See tests for how to use it.
/// @tparam L The length of the array. It should be automatically deduced.
template<size_t L>
class MakeAssignable
{
public:
    explicit MakeAssignable(char (&dest)[L]) : m_dest(dest)
    {
        static_assert(L > 0, "L should be greater than 0");
    }

    MakeAssignable& operator=(const std::string& src)
    {
        strncpy(m_dest, src.c_str(), L - 1);
        m_dest[src.size() < L ? src.size() : L - 1] = '\0';

        return *this;
    }

    MakeAssignable& operator+=(const std::string& src)
    {
        strncat(m_dest, src.c_str(), L - strlen(m_dest) - 1);
        m_dest[strlen(m_dest) + src.size() < L ? strlen(m_dest) + src.size() : L - 1] = '\0';

        return *this;
    }

private:
    char (&m_dest)[L];
};

/// Make a char array assignable with = and +=.
#define M_A trade::utilities::MakeAssignable

} // namespace trade::utilities
