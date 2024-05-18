#include <catch.hpp>
#include <thread>

#include "utilities/LoginSyncer.hpp"

class MockLoginSyncer final: public trade::utilities::LoginSyncer
{
    /// You should use LoginSyncer like this in your code.
};

TEST_CASE("Login/Logout synchronization", "[LoginSyncer]")
{
    SECTION("Successful login")
    {
        MockLoginSyncer mock_login_syncer;

        mock_login_syncer.start_login();

        std::thread notifier_thread([&mock_login_syncer] {
            mock_login_syncer.notify_login_success();
        });

        CHECK_NOTHROW(mock_login_syncer.wait_login());

        notifier_thread.join();
    }

    SECTION("Unsuccessful login")
    {
        MockLoginSyncer mock_login_syncer;

        mock_login_syncer.start_login();

        std::thread notifier_thread([&mock_login_syncer] {
            mock_login_syncer.notify_login_failure("Artificial failure with code {}", 42);
        });

        CHECK_THROWS_AS(mock_login_syncer.wait_login(), std::runtime_error);

        notifier_thread.join();
    }

    SECTION("Successful Logout")
    {
        MockLoginSyncer mock_login_syncer;

        mock_login_syncer.start_logout();

        std::thread notifier_thread([&mock_login_syncer] {
            mock_login_syncer.notify_logout_success();
        });

        CHECK_NOTHROW(mock_login_syncer.wait_logout());

        notifier_thread.join();
    }

    SECTION("Unsuccessful Logout")
    {
        MockLoginSyncer mock_login_syncer;

        mock_login_syncer.start_logout();

        std::thread notifier_thread([&mock_login_syncer] {
            mock_login_syncer.notify_logout_failure("Artificial failure with code {}", 42);
        });

        CHECK_THROWS_AS(mock_login_syncer.wait_logout(), std::runtime_error);

        notifier_thread.join();
    }
}
