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

TEST_CASE("ConfigParser: Parse int32_t", "[config][parser]")
{
    ConfigParser parser;

    auto result1 = parser.parse<int32_t>("42");
    auto result2 = parser.parse<int32_t>("-100");
    auto result3 = parser.parse<int32_t>("2147483647");
    auto result4 = parser.parse<int32_t>("-2147483648");

    REQUIRE(result1.has_value());
    REQUIRE(*result1 == 42);
    REQUIRE(*result2 == -100);
    REQUIRE(*result3 == 2147483647);
    REQUIRE(*result4 == -2147483648);
}

TEST_CASE("ConfigParser: Parse int32_t - out of range", "[config][parser]")
{
    ConfigParser parser;

    auto result1 = parser.parse<int32_t>("2147483648");
    auto result2 = parser.parse<int32_t>("-2147483649");

    REQUIRE(!result1.has_value());
    REQUIRE(result1.error() == ConfigError::ParseError);
    REQUIRE(!result2.has_value());
    REQUIRE(result2.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse int32_t - invalid value", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<int32_t>("not a number");
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse uint32_t", "[config][parser]")
{
    ConfigParser parser;

    auto result1 = parser.parse<uint32_t>("0");
    auto result2 = parser.parse<uint32_t>("42");
    auto result3 = parser.parse<uint32_t>("4294967295");

    REQUIRE(result1.has_value());
    REQUIRE(*result1 == 0);
    REQUIRE(*result2 == 42);
    REQUIRE(*result3 == 4294967295);
}

TEST_CASE("ConfigParser: Parse uint32_t - out of range", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<uint32_t>("4294967296");
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse uint32_t - negative value", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.parse<uint32_t>("-1");
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigParser: Parse bool vectors", "[config][parser]")
{
    ConfigParser parser;

    auto vec2 = parser.parse<glm::bvec2>("true, false");
    auto vec3 = parser.parse<glm::bvec3>("true, false, true");
    auto vec4 = parser.parse<glm::bvec4>("false, true, false, true");

    REQUIRE(vec2.has_value());
    REQUIRE((*vec2)[0] == true);
    REQUIRE((*vec2)[1] == false);

    REQUIRE(vec3.has_value());
    REQUIRE((*vec3)[0] == true);
    REQUIRE((*vec3)[1] == false);
    REQUIRE((*vec3)[2] == true);

    REQUIRE(vec4.has_value());
    REQUIRE((*vec4)[0] == false);
    REQUIRE((*vec4)[1] == true);
    REQUIRE((*vec4)[2] == false);
    REQUIRE((*vec4)[3] == true);
}

TEST_CASE("ConfigParser: Parse int vectors", "[config][parser]")
{
    ConfigParser parser;

    auto vec2 = parser.parse<glm::ivec2>("1, -2");
    auto vec3 = parser.parse<glm::ivec3>("1, -2, 3");
    auto vec4 = parser.parse<glm::ivec4>("1, -2, 3, -4");

    REQUIRE(vec2.has_value());
    REQUIRE((*vec2)[0] == 1);
    REQUIRE((*vec2)[1] == -2);

    REQUIRE(vec3.has_value());
    REQUIRE((*vec3)[0] == 1);
    REQUIRE((*vec3)[1] == -2);
    REQUIRE((*vec3)[2] == 3);

    REQUIRE(vec4.has_value());
    REQUIRE((*vec4)[0] == 1);
    REQUIRE((*vec4)[1] == -2);
    REQUIRE((*vec4)[2] == 3);
    REQUIRE((*vec4)[3] == -4);
}

TEST_CASE("ConfigParser: Parse uint vectors", "[config][parser]")
{
    ConfigParser parser;

    auto vec2 = parser.parse<glm::uvec2>("1, 2");
    auto vec3 = parser.parse<glm::uvec3>("1, 2, 3");
    auto vec4 = parser.parse<glm::uvec4>("1, 2, 3, 4");

    REQUIRE(vec2.has_value());
    REQUIRE((*vec2)[0] == 1);
    REQUIRE((*vec2)[1] == 2);

    REQUIRE(vec3.has_value());
    REQUIRE((*vec3)[0] == 1);
    REQUIRE((*vec3)[1] == 2);
    REQUIRE((*vec3)[2] == 3);

    REQUIRE(vec4.has_value());
    REQUIRE((*vec4)[0] == 1);
    REQUIRE((*vec4)[1] == 2);
    REQUIRE((*vec4)[2] == 3);
    REQUIRE((*vec4)[3] == 4);
}

TEST_CASE("ConfigParser: Parse vectors - invalid component count", "[config][parser]")
{
    ConfigParser parser;

    REQUIRE(!parser.parse<glm::vec2>("1.0").has_value());
    REQUIRE(!parser.parse<glm::vec2>("1.0, 2.0, 3.0").has_value());
    REQUIRE(!parser.parse<glm::ivec3>("1, 2").has_value());
    REQUIRE(!parser.parse<glm::uvec4>("1, 2, 3").has_value());
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

TEST_CASE("ConfigParser: to_string - float", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.to_string(3.14159f);

    REQUIRE(result == "3.141590");
}

TEST_CASE("ConfigParser: to_string - int", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.to_string(-42);

    REQUIRE(result == "-42");
}

TEST_CASE("ConfigParser: to_string - uint", "[config][parser]")
{
    ConfigParser parser;

    auto result = parser.to_string(42u);

    REQUIRE(result == "42");
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

    glm::vec2 value {1.5f, 2.5f};
    auto result = parser.to_string(value);

    REQUIRE(result == "1.500000,2.500000");
}

TEST_CASE("ConfigParser: to_string - vec3", "[config][parser]")
{
    ConfigParser parser;

    glm::vec3 value {1.0f, 2.0f, 3.0f};
    auto result = parser.to_string(value);

    REQUIRE(result == "1.000000,2.000000,3.000000");
}

TEST_CASE("ConfigParser: to_string - vec4", "[config][parser]")
{
    ConfigParser parser;

    glm::vec4 value {1.0f, 2.0f, 3.0f, 4.0f};
    auto result = parser.to_string(value);

    REQUIRE(result == "1.000000,2.000000,3.000000,4.000000");
}

TEST_CASE("ConfigParser: to_string - ivec2", "[config][parser]")
{
    ConfigParser parser;

    glm::ivec2 value {10, -20};
    auto result = parser.to_string(value);

    REQUIRE(result == "10,-20");
}

TEST_CASE("ConfigParser: to_string - ivec3", "[config][parser]")
{
    ConfigParser parser;

    glm::ivec3 value {10, -20, 30};
    auto result = parser.to_string(value);

    REQUIRE(result == "10,-20,30");
}

TEST_CASE("ConfigParser: to_string - ivec4", "[config][parser]")
{
    ConfigParser parser;

    glm::ivec4 value {10, -20, 30, -40};
    auto result = parser.to_string(value);

    REQUIRE(result == "10,-20,30,-40");
}

TEST_CASE("ConfigParser: to_string - uvec2", "[config][parser]")
{
    ConfigParser parser;

    glm::uvec2 value {10u, 20u};
    auto result = parser.to_string(value);

    REQUIRE(result == "10,20");
}

TEST_CASE("ConfigParser: to_string - uvec3", "[config][parser]")
{
    ConfigParser parser;

    glm::uvec3 value {10u, 20u, 30u};
    auto result = parser.to_string(value);

    REQUIRE(result == "10,20,30");
}

TEST_CASE("ConfigParser: to_string - uvec4", "[config][parser]")
{
    ConfigParser parser;

    glm::uvec4 value {10u, 20u, 30u, 40u};
    auto result = parser.to_string(value);

    REQUIRE(result == "10,20,30,40");
}

TEST_CASE("ConfigParser: to_string - bvec2", "[config][parser]")
{
    ConfigParser parser;

    glm::bvec2 value {true, false};
    auto result = parser.to_string(value);

    REQUIRE(result == "true,false");
}

TEST_CASE("ConfigParser: to_string - bvec3", "[config][parser]")
{
    ConfigParser parser;

    glm::bvec3 value {true, false, true};
    auto result = parser.to_string(value);

    REQUIRE(result == "true,false,true");
}

TEST_CASE("ConfigParser: to_string - bvec4", "[config][parser]")
{
    ConfigParser parser;

    glm::bvec4 value {true, false, true, false};
    auto result = parser.to_string(value);

    REQUIRE(result == "true,false,true,false");
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
