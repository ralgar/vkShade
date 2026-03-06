#include "config/config_parser.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace vkShade;

TEST_CASE("ConfigParser: Parse string", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<std::string>("hello world");
    REQUIRE(result.has_value());
    REQUIRE(*result == "hello world");
}

TEST_CASE("ConfigParser: Parse bool - true values", "[config][parser]")
{
    ConfigParser parser;

    REQUIRE(*parser.parse<bool>("true") == true);
    REQUIRE(*parser.parse<bool>("True") == true);
    REQUIRE(*parser.parse<bool>("TRUE") == true);
    REQUIRE(*parser.parse<bool>("1") == true);
    REQUIRE(*parser.parse<bool>("yes") == true);
    REQUIRE(*parser.parse<bool>("on") == true);
}

TEST_CASE("ConfigParser: Parse bool - false values", "[config][parser]")
{
    ConfigParser parser;

    REQUIRE(*parser.parse<bool>("false") == false);
    REQUIRE(*parser.parse<bool>("False") == false);
    REQUIRE(*parser.parse<bool>("FALSE") == false);
    REQUIRE(*parser.parse<bool>("0") == false);
    REQUIRE(*parser.parse<bool>("no") == false);
    REQUIRE(*parser.parse<bool>("off") == false);
}

TEST_CASE("ConfigParser: Parse bool - invalid value", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<bool>("maybe");
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse float", "[config][parser]")
{
    ConfigParser parser;

    auto result1 = parser.parse<float>("3.14159");
    auto result2 = parser.parse<float>("-2.5");
    auto result3 = parser.parse<float>("0.0");

    REQUIRE(result1.has_value());
    REQUIRE_THAT(*result1, Catch::Matchers::WithinRel(3.14159f, 0.0001f));
    REQUIRE_THAT(*result2, Catch::Matchers::WithinRel(-2.5f, 0.0001f));
    REQUIRE_THAT(*result3, Catch::Matchers::WithinRel(0.0f, 0.0001f));
}

TEST_CASE("ConfigParser: Parse float - invalid value", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<float>("not a number");
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse vec2", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<glm::vec2>("1.5, 2.5");
    REQUIRE(result.has_value());
    REQUIRE_THAT(result->x, Catch::Matchers::WithinRel(1.5f, 0.0001f));
    REQUIRE_THAT(result->y, Catch::Matchers::WithinRel(2.5f, 0.0001f));
}

TEST_CASE("ConfigParser: Parse vec2 - invalid component count", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<glm::vec2>("1.5, 2.5, 3.5");
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse vec3", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<glm::vec3>("1.0, 2.0, 3.0");
    REQUIRE(result.has_value());
    REQUIRE_THAT(result->x, Catch::Matchers::WithinRel(1.0f, 0.0001f));
    REQUIRE_THAT(result->y, Catch::Matchers::WithinRel(2.0f, 0.0001f));
    REQUIRE_THAT(result->z, Catch::Matchers::WithinRel(3.0f, 0.0001f));
}

TEST_CASE("ConfigParser: Parse vec4", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<glm::vec4>("1.0, 2.0, 3.0, 4.0");
    REQUIRE(result.has_value());
    REQUIRE_THAT(result->x, Catch::Matchers::WithinRel(1.0f, 0.0001f));
    REQUIRE_THAT(result->y, Catch::Matchers::WithinRel(2.0f, 0.0001f));
    REQUIRE_THAT(result->z, Catch::Matchers::WithinRel(3.0f, 0.0001f));
    REQUIRE_THAT(result->w, Catch::Matchers::WithinRel(4.0f, 0.0001f));
}

TEST_CASE("ConfigParser: Parse string list", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<std::vector<std::string>>("item1, item2, item3");
    REQUIRE(result.has_value());
    REQUIRE(result->size() == 3);
    REQUIRE((*result)[0] == "item1");
    REQUIRE((*result)[1] == "item2");
    REQUIRE((*result)[2] == "item3");
}

TEST_CASE("ConfigParser: Parse string list - handles whitespace", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<std::vector<std::string>>("  item1  ,  item2  ,  item3  ");
    REQUIRE(result.has_value());
    REQUIRE(result->size() == 3);
    REQUIRE((*result)[0] == "item1");
    REQUIRE((*result)[1] == "item2");
    REQUIRE((*result)[2] == "item3");
}

TEST_CASE("ConfigParser: Parse string list - empty string", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<std::vector<std::string>>("");
    REQUIRE(result.has_value());
    REQUIRE(result->empty());
}

TEST_CASE("ConfigParser: to_string - string", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.to_string(std::string("test"));
    REQUIRE(result == "test");
}

TEST_CASE("ConfigParser: to_string - const char*", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.to_string("test");
    REQUIRE(result == "test");
}

TEST_CASE("ConfigParser: to_string - bool", "[config][parser]")
{
    ConfigParser parser;

    auto result1 = parser.to_string(true);
    auto result2 = parser.to_string(false);
    REQUIRE(result1 == "true");
    REQUIRE(result2 == "false");
}

TEST_CASE("ConfigParser: to_string - vec2", "[config][parser]")
{
    ConfigParser parser;

    glm::vec2 v{1.5f, 2.5f};
    auto result = parser.to_string(v);
    REQUIRE(result == "1.500000,2.500000");
}

TEST_CASE("ConfigParser: to_string - vec3", "[config][parser]")
{
    ConfigParser parser;

    glm::vec3 v{1.0f, 2.0f, 3.0f};
    auto result = parser.to_string(v);
    REQUIRE(result == "1.000000,2.000000,3.000000");
}

TEST_CASE("ConfigParser: to_string - string list", "[config][parser]")
{
    ConfigParser parser;

    std::vector<std::string> items = {"a", "b", "c"};
    auto result = parser.to_string(items);
    REQUIRE(result == "a,b,c");
}

TEST_CASE("ConfigParser: to_string - empty string list", "[config][parser]")
{
    ConfigParser parser;

    std::vector<std::string> items;
    auto result = parser.to_string(items);
    REQUIRE(result == "");
}
