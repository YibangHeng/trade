#pragma once

#if __GNUC__ >= 4
    #define TD_PUBLIC_API __attribute__((visibility("default")))
    #define LOCAL_API __attribute__((visibility("hidden")))
#else
    #define TD_PUBLIC_API
    #define LOCAL_API
#endif
