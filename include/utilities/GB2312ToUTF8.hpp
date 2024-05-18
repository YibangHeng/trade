#pragma once

#include <string>

#ifdef unix
    #include <iconv.h>
#endif

#ifdef WIN32
    #include <windows.h>
#endif

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
#ifdef unix
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
#endif

#ifdef WIN32
        // Convert GB2312 string to a wide-character string.
        const int wide_chars_num = MultiByteToWideChar(CP_ACP, 0, gb2312_string.c_str(), -1, nullptr, 0);
        if (wide_chars_num == 0) {
            throw std::runtime_error("MultiByteToWideChar failed");
        }

        std::wstring wide_string(wide_chars_num, 0);
        MultiByteToWideChar(CP_ACP, 0, gb2312_string.c_str(), -1, &wide_string[0], wide_chars_num);

        // Convert wide-character string to UTF-8 string.
        const int utf8_chars_num = WideCharToMultiByte(CP_UTF8, 0, wide_string.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8_chars_num == 0) {
            throw std::runtime_error("WideCharToMultiByte failed");
        }

        std::string utf8_string(utf8_chars_num, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide_string.c_str(), -1, &utf8_string[0], utf8_chars_num, nullptr, nullptr);

        return utf8_string;
#endif
    }
};

} // namespace trade::utilities
