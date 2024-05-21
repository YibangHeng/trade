#include <catch.hpp>
#include <thread>

#include "utilities/NetworkHelper.hpp"

constexpr size_t insertion_times = 16;
constexpr size_t insertion_batch = 1024;

TEST_CASE("Message with message_id == 0 and empty message", "[Serializer]")
{
    const trade::types::EmptyMessage empty_message;

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::invalid_message_id,
        empty_message
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    CHECK(message_id == trade::types::MessageID::invalid_message_id);
}

TEST_CASE("Message with message_id != 0 and empty message", "[Serializer]")
{
    const trade::types::EmptyMessage empty_message;

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::unix_sig,
        empty_message
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    CHECK(message_id == trade::types::MessageID::unix_sig);
}

TEST_CASE("Message with message_id == 0 and non-empty message", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::invalid_message_id,
        unix_sig
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    unix_sig.Clear();
    unix_sig.ParseFromString(message_body);

    CHECK(message_id == trade::types::MessageID::invalid_message_id);
    CHECK(unix_sig.sig() == 2);
}

TEST_CASE("Message with message_id != 0 and non-empty message", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::unix_sig,
        unix_sig
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    unix_sig.Clear();
    unix_sig.ParseFromString(message_body);

    CHECK(message_id == trade::types::MessageID::unix_sig);
    CHECK(unix_sig.sig() == 2);
}

TEST_CASE("Message with an out-of-range message_id", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        static_cast<trade::types::MessageID>(trade::types::MessageID_MAX + 1),
        unix_sig
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    unix_sig.Clear();
    unix_sig.ParseFromString(message_body);

    CHECK(message_id == trade::types::MessageID::invalid_message_id);
    CHECK(unix_sig.sig() == 2); /// The returned message still contains the original value.
}

TEST_CASE("Sending and receiving messages with M:1 connections", "[RRServer/RRClient]")
{
    trade::utilities::ZMQContextPtr zmq_context;
    zmq_context.reset(zmq_ctx_new(), trade::utilities::ZMQContextPtrDeleter());

    std::mutex check_mutex;

    /// Server side.
    auto server_worker = [&zmq_context, &check_mutex] {
        trade::utilities::RRServer server("inproc://server", zmq_context);

        for (int i = 0; i < insertion_times * insertion_batch; i++) {
            const auto [message_id, message_body] = server.receive();

            trade::types::UnixSig unix_sig;
            unix_sig.ParseFromString(message_body);

            {
                std::lock_guard lock(check_mutex);
                CHECK(message_id == trade::types::MessageID::unix_sig);
            }

            server.send(trade::types::MessageID::unix_sig, unix_sig);
        }
    };

    std::thread server_thread(server_worker);

    /// Client side.
    auto client_worker = [&zmq_context, &check_mutex] {
        trade::utilities::RRClient client("inproc://server", zmq_context);

        for (int i = 0; i < insertion_batch; i++) {
            trade::types::UnixSig send_unix_sig;
            send_unix_sig.set_sig(i);

            const auto received_unix_sig = client.request<trade::types::UnixSig>(trade::types::MessageID::unix_sig, send_unix_sig);

            {
                std::lock_guard lock(check_mutex);
                CHECK(received_unix_sig.sig() == i);
            }
        }
    };

    std::array<std::thread, insertion_times> client_threads;

    for (auto& client_thread : client_threads)
        client_thread = std::thread(client_worker);

    for (auto& client_thread : client_threads)
        client_thread.join();

    server_thread.join();
}

TEST_CASE("Sending and receiving messages via IP multicast", "[MCServer/MCClient]")
{
    const std::string multicast_address = "239.255.255.255";
    constexpr uint16_t multicast_port   = 5555;

    /// Record the number of times each signal is received. The index is the
    /// value of the signal, and the value is the number of receptions of the
    /// signal.
    std::array<std::atomic<size_t>, insertion_times * insertion_batch> touched_times;

    /// Server side.
    auto server_worker = [multicast_address] {
        trade::utilities::MCServer server(multicast_address, multicast_port);

        /// Wait for all clients to connect.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        for (int i = 0; i < insertion_times * insertion_batch; i++) {
            trade::types::UnixSig unix_sig;
            unix_sig.set_sig(i);

            server.send(trade::utilities::Serializer::serialize(trade::types::MessageID::unix_sig, unix_sig));
        }
    };

    std::thread server_thread(server_worker);

    /// Client side.
    auto client_worker = [multicast_address, &touched_times] {
        trade::utilities::MCClient client(multicast_address, multicast_port);

        for (int i = 0; i < insertion_times * insertion_batch; i++) {
            const auto received_unix_sig          = client.receive();

            const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(received_unix_sig);

            trade::types::UnixSig unix_sig;
            unix_sig.ParseFromString(message_body);

            /// Record the number of received signals.
            ++touched_times[static_cast<size_t>(i)];
        }
    };

    std::array<std::thread, insertion_times> client_threads;

    for (auto& client_thread : client_threads)
        client_thread = std::thread(client_worker);

    for (auto& client_thread : client_threads)
        client_thread.join();

    server_thread.join();

    /// Check whether all signals are received, no duplication or loss.
    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        CHECK(touched_times[static_cast<size_t>(i)] == insertion_times);
    }
}
