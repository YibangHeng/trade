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
    const auto sse_l2_snap_0 = std::make_shared<trade::types::L2Tick>();

    sse_l2_snap_0->set_symbol("600875.SH");
    sse_l2_snap_0->set_price_1000x(2233);
    sse_l2_snap_0->set_quantity(1000);
    sse_l2_snap_0->set_ask_unique_id(10001);
    sse_l2_snap_0->set_bid_unique_id(10002);
    sse_l2_snap_0->set_exchange_time(925000);

    const auto sse_l2_snap_1 = std::make_shared<trade::types::L2Tick>();

    sse_l2_snap_1->set_symbol("600875.SH");
    sse_l2_snap_1->set_price_1000x(2233);
    sse_l2_snap_1->set_quantity(2000);
    sse_l2_snap_1->set_ask_unique_id(20001);
    sse_l2_snap_1->set_bid_unique_id(20002);
    sse_l2_snap_1->set_exchange_time(925000);

    const auto sse_l2_snap_2 = std::make_shared<trade::types::L2Tick>();

    sse_l2_snap_2->set_symbol("600875.SH");
    sse_l2_snap_2->set_price_1000x(2233);
    sse_l2_snap_2->set_quantity(3000);
    sse_l2_snap_2->set_ask_unique_id(30001);
    sse_l2_snap_2->set_bid_unique_id(30002);
    sse_l2_snap_2->set_exchange_time(925000);

    const auto szse_l2_snap_0 = std::make_shared<trade::types::L2Tick>();

    szse_l2_snap_0->set_symbol("000001.SZ");
    szse_l2_snap_0->set_price_1000x(3322);
    szse_l2_snap_0->set_quantity(1000);
    szse_l2_snap_0->set_ask_unique_id(10001);
    szse_l2_snap_0->set_bid_unique_id(10002);
    szse_l2_snap_0->set_exchange_time(925000);

    const auto szse_l2_snap_1 = std::make_shared<trade::types::L2Tick>();

    szse_l2_snap_1->set_symbol("000001.SZ");
    szse_l2_snap_1->set_price_1000x(3322);
    szse_l2_snap_1->set_quantity(2000);
    szse_l2_snap_1->set_ask_unique_id(20001);
    szse_l2_snap_1->set_bid_unique_id(20002);
    szse_l2_snap_1->set_exchange_time(925000);

    const auto szse_l2_snap_2 = std::make_shared<trade::types::L2Tick>();

    szse_l2_snap_2->set_symbol("000001.SZ");
    szse_l2_snap_2->set_price_1000x(3322);
    szse_l2_snap_2->set_quantity(3000);
    szse_l2_snap_2->set_ask_unique_id(30001);
    szse_l2_snap_2->set_bid_unique_id(30002);
    szse_l2_snap_2->set_exchange_time(925000);

    SECTION("Sending and receiving messages with M:1 connections")
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool new_subscribe_req_acknowledged = false;

        /// Server side.
        trade::reporter::SubReporter sub_reporter(10000);

        /// Client side.
        auto client_worker = [&mutex, &cv, &new_subscribe_req_acknowledged] {
            std::atomic<size_t> l2_snap_counter = 0;

            trade::SubReporterClientImpl client(
                "127.0.0.1",
                10000,
                [&l2_snap_counter](const muduo::net::TcpConnectionPtr&, const trade::types::L2Tick& l2_tick, muduo::Timestamp) {
                    l2_snap_counter++;

                    CHECK(l2_tick.symbol() == "600875.SH");
                    CHECK(l2_tick.price_1000x() == 2233);
                    CHECK(l2_tick.quantity() == 1000 * l2_snap_counter);
                    CHECK(l2_tick.ask_unique_id() == l2_snap_counter * 10000 + 1);
                    CHECK(l2_tick.bid_unique_id() == l2_snap_counter * 10000 + 2);
                    CHECK(l2_tick.exchange_time() == 925000);
                },
                [&new_subscribe_req_acknowledged, &mutex, &cv](const muduo::net::TcpConnectionPtr&, const trade::types::NewSubscribeRsp& new_subscribe_rsp, muduo::Timestamp) {
                    CHECK(new_subscribe_rsp.subscribed_symbol().size() == 1);
                    CHECK(new_subscribe_rsp.subscribed_symbol().at(0) == "600875.SH");

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

            client.subscribe<std::initializer_list<std::string>>({"600875.SH"}, {"000001.SZ"});

            while (l2_snap_counter != 3) {
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

        sub_reporter.exchange_l2_tick_arrived(sse_l2_snap_0);
        sub_reporter.exchange_l2_tick_arrived(sse_l2_snap_1);
        sub_reporter.exchange_l2_tick_arrived(sse_l2_snap_2);

        /// This l2 snap will not be sent to the client since it is not subscribed.
        sub_reporter.exchange_l2_tick_arrived(szse_l2_snap_0);
        sub_reporter.exchange_l2_tick_arrived(szse_l2_snap_1);
        sub_reporter.exchange_l2_tick_arrived(szse_l2_snap_2);

        for (auto& client_thread : client_threads)
            client_thread.join();
    }
}
