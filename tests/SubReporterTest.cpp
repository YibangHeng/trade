#include <array>
#include <atomic>
#include <catch.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "AppBase.hpp"
#include "auxiliaries/sub_reporter_client/SubReporterClientImpl.hpp"
#include "libreporter/SubReporter.h"

constexpr size_t num_threads = 1;

TEST_CASE("Communication between SubReporterServer and SubReporterClientImpl", "[SubReporterClientImpl]")
{
    const auto sse_l2_snap_0 = std::make_shared<trade::types::ExchangeL2Snap>();

    sse_l2_snap_0->set_symbol("600875");
    sse_l2_snap_0->set_price(1000);
    sse_l2_snap_0->set_exchange_time(925000);

    const auto sse_l2_snap_1 = std::make_shared<trade::types::ExchangeL2Snap>();

    sse_l2_snap_1->set_symbol("600875");
    sse_l2_snap_1->set_price(2000);
    sse_l2_snap_1->set_exchange_time(925000);

    const auto sse_l2_snap_2 = std::make_shared<trade::types::ExchangeL2Snap>();

    sse_l2_snap_2->set_symbol("600875");
    sse_l2_snap_2->set_price(3000);
    sse_l2_snap_2->set_exchange_time(925000);

    const auto szse_l2_snap_0 = std::make_shared<trade::types::ExchangeL2Snap>();

    szse_l2_snap_0->set_symbol("000001");
    szse_l2_snap_0->set_price(4000);
    szse_l2_snap_0->set_exchange_time(925000);

    const auto szse_l2_snap_1 = std::make_shared<trade::types::ExchangeL2Snap>();

    szse_l2_snap_1->set_symbol("000001");
    szse_l2_snap_1->set_price(5000);
    szse_l2_snap_1->set_exchange_time(925000);

    const auto szse_l2_snap_2 = std::make_shared<trade::types::ExchangeL2Snap>();

    szse_l2_snap_2->set_symbol("000001");
    szse_l2_snap_2->set_price(6000);
    szse_l2_snap_2->set_exchange_time(925000);

    SECTION("Subscribing for specific symbols")
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool new_subscribe_req_acknowledged = false;

        /// Server side.
        trade::reporter::SubReporter sub_reporter(10100);

        /// Client side.
        auto client_worker = [&mutex, &cv, &new_subscribe_req_acknowledged] {
            std::atomic<size_t> l2_snap_counter = 0;

            trade::SubReporterClientImpl client(
                "127.0.0.1",
                10100,
                [&l2_snap_counter](const muduo::net::TcpConnectionPtr&, const trade::types::ExchangeL2Snap& exchange_l2_snap, muduo::Timestamp) {
                    l2_snap_counter++;

                    CHECK(exchange_l2_snap.symbol() == "600875");
                    CHECK(exchange_l2_snap.price() == 1000 * l2_snap_counter);
                    CHECK(exchange_l2_snap.exchange_time() == 925000);
                },
                [&new_subscribe_req_acknowledged, &mutex, &cv](const muduo::net::TcpConnectionPtr&, const trade::types::NewSubscribeRsp& new_subscribe_rsp, muduo::Timestamp) {
                    CHECK(new_subscribe_rsp.subscribed_symbols().size() == 1);
                    CHECK(new_subscribe_rsp.subscribed_symbols().at(0) == "600875");

                    std::lock_guard lock(mutex);
                    new_subscribe_req_acknowledged = true;
                    cv.notify_one();
                },
                [](const muduo::net::TcpConnectionPtr& conn) {
                    CHECK(conn != nullptr);
                },
                [](const muduo::net::TcpConnectionPtr& conn) {
                    CHECK(conn == nullptr);
                }
            );

            client.wait_login();

            client.subscribe<std::initializer_list<std::string>>({"600875"}, {"000001"});

            while (l2_snap_counter < 3) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        };

        std::array<std::thread, num_threads> client_threads;

        for (auto& client_thread : client_threads) {
            client_thread = std::thread(client_worker);
        }

        /// Wait for subscription acknowledgement.
        std::unique_lock lock(mutex);
        cv.wait(lock, [&new_subscribe_req_acknowledged] { return new_subscribe_req_acknowledged; });

        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_0);
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_1);
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_2);

        /// This l2 snap will not be sent to the client since it is not subscribed.
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_0);
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_1);
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_2);

        for (auto& client_thread : client_threads)
            client_thread.join();
    }

    SECTION("Subscribing for all symbols")
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool new_subscribe_req_acknowledged = false;

        /// Server side.
        trade::reporter::SubReporter sub_reporter(10100);

        /// Client side.
        auto client_worker = [&mutex, &cv, &new_subscribe_req_acknowledged] {
            std::atomic<size_t> l2_snap_counter = 0;

            trade::SubReporterClientImpl client(
                "127.0.0.1",
                10100,
                [&l2_snap_counter](const muduo::net::TcpConnectionPtr&, const trade::types::ExchangeL2Snap& exchange_l2_snap, muduo::Timestamp) {
                    l2_snap_counter++;

                    if (l2_snap_counter < 4) {
                        CHECK(exchange_l2_snap.symbol() == "600875");
                        CHECK(exchange_l2_snap.price() == 1000 * l2_snap_counter);
                        CHECK(exchange_l2_snap.exchange_time() == 925000);
                    }
                    else {
                        CHECK(exchange_l2_snap.symbol() == "000001");
                        CHECK(exchange_l2_snap.price() == 1000 * l2_snap_counter);
                        CHECK(exchange_l2_snap.exchange_time() == 925000);
                    }
                },
                [&new_subscribe_req_acknowledged, &mutex, &cv](const muduo::net::TcpConnectionPtr&, const trade::types::NewSubscribeRsp& new_subscribe_rsp, muduo::Timestamp) {
                    CHECK(new_subscribe_rsp.subscribed_symbols().size() == 1);
                    CHECK(new_subscribe_rsp.subscribed_symbols().at(0) == "*");

                    std::lock_guard lock(mutex);
                    new_subscribe_req_acknowledged = true;
                    cv.notify_one();
                },
                [](const muduo::net::TcpConnectionPtr& conn) {
                    CHECK(conn != nullptr);
                },
                [](const muduo::net::TcpConnectionPtr& conn) {
                    CHECK(conn == nullptr);
                }
            );

            client.wait_login();

            client.subscribe<std::initializer_list<std::string>>({"*"}, {});

            while (l2_snap_counter < 6) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        };

        std::array<std::thread, num_threads> client_threads;

        for (auto& client_thread : client_threads) {
            client_thread = std::thread(client_worker);
        }

        /// Wait for subscription acknowledgement.
        std::unique_lock lock(mutex);
        cv.wait(lock, [&new_subscribe_req_acknowledged] { return new_subscribe_req_acknowledged; });

        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_0);
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_1);
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_2);

        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_0);
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_1);
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_2);

        for (auto& client_thread : client_threads)
            client_thread.join();
    }

    SECTION("Requesting for previous l2 ticks")
    {
        /// Server side.
        trade::reporter::SubReporter sub_reporter(10100);

        /// Report before client connected.
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_0);
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_1);
        sub_reporter.exchange_l2_snap_arrived(sse_l2_snap_2);

        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_0);
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_1);
        sub_reporter.exchange_l2_snap_arrived(szse_l2_snap_2);

        /// Client side.
        auto client_worker = [] {
            std::atomic<size_t> l2_snap_counter = 0;

            trade::SubReporterClientImpl client(
                "127.0.0.1",
                10100,
                [&l2_snap_counter](const muduo::net::TcpConnectionPtr&, const trade::types::ExchangeL2Snap& exchange_l2_snap, muduo::Timestamp) {
                    l2_snap_counter++;

                    if (l2_snap_counter == 1) {
                        CHECK(exchange_l2_snap.symbol() == "000001");
                        CHECK(exchange_l2_snap.price() == 6000);
                        CHECK(exchange_l2_snap.exchange_time() == 925000);
                    }
                    else {
                        CHECK(exchange_l2_snap.symbol() == "600875");
                        CHECK(exchange_l2_snap.price() == 3000);
                        CHECK(exchange_l2_snap.exchange_time() == 925000);
                    }
                },
                [](const muduo::net::TcpConnectionPtr&, const trade::types::NewSubscribeRsp& new_subscribe_rsp, muduo::Timestamp) {
                    CHECK(new_subscribe_rsp.subscribed_symbols().size() == 1);
                    CHECK(new_subscribe_rsp.subscribed_symbols().at(0) == "*");
                },
                [](const muduo::net::TcpConnectionPtr& conn) {
                    CHECK(conn != nullptr);
                },
                [](const muduo::net::TcpConnectionPtr& conn) {
                    CHECK(conn == nullptr);
                }
            );

            client.wait_login();

            client.subscribe<std::initializer_list<std::string>>({"*"}, {}, true);

            while (l2_snap_counter < 2) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        };

        std::array<std::thread, num_threads> client_threads;

        for (auto& client_thread : client_threads) {
            client_thread = std::thread(client_worker);
        }

        for (auto& client_thread : client_threads)
            client_thread.join();
    }
}
