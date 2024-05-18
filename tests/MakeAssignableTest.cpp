#include <catch.hpp>

#include "utilities/MakeAssignable.hpp"

TEST_CASE("MakeAssignable", "[MakeAssignable]")
{
    SECTION("Normal assignment")
    {
        char str[16] = {};

        M_A {str}    = "";
        CHECK(strcmp(str, "") == 0);

        M_A {str} = "0";
        CHECK(strcmp(str, "0") == 0);

        M_A {str} = "Hello, world!";
        CHECK(strcmp(str, "Hello, world!") == 0);
    }

    SECTION("Overflowed assignment")
    {
        char str[16] = {};

        M_A {str}    = "123456789ABCDEF";
        CHECK(strcmp(str, "123456789ABCDEF") == 0);

        /// For overflowed assignment.
        /// It should assign as many as possible.
        M_A {str} = "123456789ABCDEF0";
        CHECK(strcmp(str, "123456789ABCDEF") == 0);
        CHECK(str[15] == '\0');
    }

    SECTION("Append assignment")
    {
        char str[16] = {};

        M_A {str} += "";
        CHECK(strcmp(str, "") == 0);

        M_A {str} += "12345";
        CHECK(strcmp(str, "12345") == 0);

        M_A {str} += "6789";
        CHECK(strcmp(str, "123456789") == 0);

        /// For overflowed append.
        /// It should append as many as possible.
        M_A {str} += "ABCDEF";
        CHECK(strcmp(str, "123456789ABCDEF") == 0);
        CHECK(str[15] == '\0');

        M_A {str} += "0";
        CHECK(strcmp(str, "123456789ABCDEF") == 0);
        CHECK(str[15] == '\0');
    }
}
