#include <catch2/catch_session.hpp>
#include <spdlog/spdlog.h>

int main(const int argc, char* argv[])
{
    spdlog::default_logger()->set_level(spdlog::level::warn);

    return Catch::Session().run(argc, argv);
}
