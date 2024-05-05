#pragma once

#include <boost/logic/tribool.hpp>
#include <condition_variable>
#include <fmt/format.h>

/// TODO: Add timeout support.

namespace trade::utilities
{

class LoginSyncerImpl
{
public:
    LoginSyncerImpl() : m_attempting(boost::logic::tribool::indeterminate_value) {}
    ~LoginSyncerImpl() = default;

public:
    void start()
    {
        m_mutex.lock();
    }

    void wait() noexcept(false)
    {
        std::unique_lock lock(m_mutex);

        /// Wait until attempting is finished.
        m_cv.wait(lock, [this] {
            return !indeterminate(m_attempting);
        });

        /// Throw exception if attemption failed.
        if (!m_attempting.load()) {
            throw std::runtime_error(m_message);
        }
    }

    void notify_success()
    {
        m_attempting.store(boost::logic::tribool::true_value);
        m_mutex.unlock();
        m_cv.notify_all();
    }

    template<typename... T>
    void notify_failure(fmt::format_string<T...> fmt, T&&... args)
    {
        m_attempting.store(boost::logic::tribool::false_value);
        m_message = fmt::format(fmt, std::forward<T>(args)...);
        m_mutex.unlock();
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<boost::logic::tribool> m_attempting;
    /// Store message about failure reason (if the caller could provide it).
    std::string m_message;
};

/// Helper class for synchronizing login/logout attempts.
class LoginSyncer
{
public:
    LoginSyncer()          = default;
    virtual ~LoginSyncer() = default;

public:
    /// Start the login process.
    void start_login()
    {
        m_login_syncer.start();
    }

    /// Start the logout process.
    void start_logout()
    {
        m_logout_syncer.start();
    }

    /// Waits until the login is complete.
    /// @throws std::runtime_error with the error message if the login fails.
    void wait_login_reasult() noexcept(false)
    {
        m_login_syncer.wait();
    }

    /// Waits until the logout is complete.
    /// @throws std::runtime_error with the error message if the logout fails.
    void wait_logout_reasult() noexcept(false)
    {
        m_logout_syncer.wait();
    }

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
