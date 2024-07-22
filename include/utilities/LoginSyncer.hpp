#pragma once

/// Disable condition variable support on Windows platforms.
#ifndef WIN32
    #define CV_SUPPORT
#endif

#include <atomic>
#include <boost/logic/tribool.hpp>
#include <condition_variable>
#include <fmt/format.h>

#include "visibility.h"

/// TODO: Add timeout support.

namespace trade::utilities
{

class LoginSyncerImpl
{
public:
#ifdef CV_SUPPORT
    LoginSyncerImpl() : m_attempting(boost::tribool::indeterminate_value) {}
#else
    LoginSyncerImpl() = default;
#endif
    ~LoginSyncerImpl() = default;

public:
    void start()
    {
#ifdef CV_SUPPORT
        m_mutex.lock();
#endif
    }

    void wait() noexcept(false)
    {
#ifdef CV_SUPPORT
        std::unique_lock lock(m_mutex);

        /// Wait until attempting is finished.
        m_cv.wait(lock, [this] {
            return !indeterminate(m_attempting);
        });

        /// Throw exception if attempting failed.
        if (!m_attempting) {
            throw std::runtime_error(m_message);
        }
#else
        /// TODO: Temporary solution. Use condition variable instead.
        std::this_thread::sleep_for(std::chrono::seconds(5));

        if (!m_message.empty()) {
            throw std::runtime_error(m_message);
        }
#endif
    }

    void notify_success()
    {
#ifdef CV_SUPPORT
        m_attempting = boost::tribool::true_value;
        m_mutex.unlock();
        m_cv.notify_all();
#endif
    }

    template<typename... T>
    void notify_failure(fmt::format_string<T...> fmt, T&&... args)
    {
#ifdef CV_SUPPORT
        m_attempting = boost::tribool::false_value;
        m_message    = fmt::format(fmt, std::forward<T>(args)...);
        m_mutex.unlock();
        m_cv.notify_all();
#else
        m_message = fmt::format(fmt, std::forward<T>(args)...);
#endif
    }

private:
#ifdef CV_SUPPORT
    std::mutex m_mutex;
    std::condition_variable m_cv;
    boost::tribool m_attempting;
#endif
    /// Store message about failure reason (if the caller could provide it).
    std::string m_message;
};

/// Helper class for synchronizing login/logout attempts.
class PUBLIC_API LoginSyncer
{
public:
    LoginSyncer()          = default;
    virtual ~LoginSyncer() = default;

public:
    /// Start the login process.
    virtual void start_login()
    {
        m_login_syncer.start();
    }

    /// Start the logout process.
    virtual void start_logout()
    {
        m_logout_syncer.start();
    }

    /// Waits until the login is complete.
    /// @throws std::runtime_error with the error message if the login fails.
    virtual void wait_login() noexcept(false)
    {
        m_login_syncer.wait();
    }

    /// Waits until the logout is complete.
    /// @throws std::runtime_error with the error message if the logout fails.
    virtual void wait_logout() noexcept(false)
    {
        m_logout_syncer.wait();
    }

public:
    /// Notifies that a login attempt has succeeded.
    void notify_login_success()
    {
        m_login_syncer.notify_success();
    }

    /// Notifies that a logout attempt has succeeded.
    void notify_logout_success()
    {
        m_logout_syncer.notify_success();
    }

    /// Notifies that a login attempt has failed with a specific message.
    /// Error message complies with fmt::format syntax.
    template<typename... T>
    void notify_login_failure(fmt::format_string<T...> fmt, T&&... args)
    {
        m_login_syncer.notify_failure(fmt, std::forward<T>(args)...);
    }

    /// Notifies that a logout attempt has failed with a specific message.
    /// Error message complies with fmt::format syntax.
    template<typename... T>
    void notify_logout_failure(fmt::format_string<T...> fmt, T&&... args)
    {
        m_logout_syncer.notify_failure(fmt, std::forward<T>(args)...);
    }

private:
    LoginSyncerImpl m_login_syncer;
    LoginSyncerImpl m_logout_syncer;
};

} // namespace trade::utilities
