#pragma once

#if __GNUC__ >= 4
    #define PUBLIC_API __attribute__((visibility("default")))
    #define LOCAL_API __attribute__((visibility("hidden")))
#else
    #define PUBLIC_API
    #define LOCAL_API
#endif
