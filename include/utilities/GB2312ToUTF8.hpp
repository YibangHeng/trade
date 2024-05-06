#pragma once

#include <iconv.h>
#include <string>

namespace trade::utilities
{

class GB2312ToUTF8
{
public:
    /// Convert GB2312 to UTF8. If the string is not GB2312, the original string
    /// is returned (for case a pure English string with no GB2312 characters).
    /// @param gb2312_string GB2312 string.
    /// @return UTF8 string.
    std::string operator()(std::string const& gb2312_string) const
    {
        /// Create an iconv converter from gb2312 to utf8.
        const auto cd = iconv_open("UTF-8", "GB2312");
        if (cd == reinterpret_cast<iconv_t>(-1)) {
            throw std::runtime_error("iconv_open failed");
        }

        /// Allocate buffer for the output string.
        size_t in_size     = gb2312_string.size();
        size_t out_size    = in_size * 4; /// Assume the worst case.
        const auto out_buf = new char[out_size];
        auto out_ptr       = out_buf;

        /// Convert the input string.
        const char* in_ptr = gb2312_string.c_str();
        if (const size_t result = iconv(cd, const_cast<char**>(&in_ptr), &in_size, &out_ptr, &out_size);
            result == static_cast<size_t>(-1)) {
            delete[] out_buf;
            iconv_close(cd);
            return gb2312_string;
        }

        /// Close the iconv converter.
        iconv_close(cd);

        /// Construct the output string.
        std::string utf8_string(out_buf, out_ptr - out_buf);
        delete[] out_buf;
        return utf8_string;
    }
};

} // namespace trade::utilities