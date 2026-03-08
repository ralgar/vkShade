#include "config/config_store.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/glm.hpp>

using namespace vkShade;

// Helper to create temporary test files
class TempConfigFile
{
public:
    TempConfigFile(const std::string& filename)
        : m_path(std::filesystem::temp_directory_path() / filename)
    {
    }

    ~TempConfigFile()
    {
        if (std::filesystem::exists(m_path))
            std::filesystem::remove(m_path);
    }

    const std::filesystem::path& path() const { return m_path; }

    void write(const std::string& content)
    {
        std::ofstream file(m_path);
        file << content;
    }

    std::string read() const
    {
        std::ifstream file(m_path);
        return std::string(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
    }

private:
    std::filesystem::path m_path;
};

TEST_CASE("ConfigStore: Set and get string", "[config][manager]")
{
    ConfigStore config;

    config.set("app", "name", std::string("MyApp"));
    auto result = config.get<std::string>("app", "name");

    REQUIRE(result.has_value());
    REQUIRE(*result == "MyApp");
}

TEST_CASE("ConfigStore: Set and get with string literal", "[config][manager]")
{
    ConfigStore config;

    config.set("app", "title", "Hello World");
    auto result = config.get<std::string>("app", "title");

    REQUIRE(result.has_value());
    REQUIRE(*result == "Hello World");
}

TEST_CASE("ConfigStore: Set and get float", "[config][manager]")
{
    ConfigStore config;

    config.set("audio", "volume", 0.75f);
    auto result = config.get<float>("audio", "volume");

    REQUIRE(result.has_value());
    REQUIRE_THAT(*result, Catch::Matchers::WithinRel(0.75f, 0.0001f));
}

TEST_CASE("ConfigStore: Set and get bool", "[config][manager]")
{
    ConfigStore config;

    config.set("graphics", "vsync", true);
    auto result = config.get<bool>("graphics", "vsync");

    REQUIRE(result.has_value());
    REQUIRE(*result == true);
}

TEST_CASE("ConfigStore: Set and get int32_t", "[config][manager]")
{
    ConfigStore config;

    config.set("graphics", "width", 1920);
    auto result = config.get<int32_t>("graphics", "width");

    REQUIRE(result.has_value());
    REQUIRE(*result == 1920);
}

TEST_CASE("ConfigStore: Set and get uint32_t", "[config][manager]")
{
    ConfigStore config;

    config.set("graphics", "samples", 4u);
    auto result = config.get<uint32_t>("graphics", "samples");

    REQUIRE(result.has_value());
    REQUIRE(*result == 4);
}

TEST_CASE("ConfigStore: Set and get vec3", "[config][manager]")
{
    ConfigStore config;

    glm::vec3 color{0.5f, 0.75f, 1.0f};
    config.set("graphics", "skyColor", color);
    auto result = config.get<glm::vec3>("graphics", "skyColor");

    REQUIRE(result.has_value());
    REQUIRE_THAT(result->x, Catch::Matchers::WithinRel(0.5f, 0.0001f));
    REQUIRE_THAT(result->y, Catch::Matchers::WithinRel(0.75f, 0.0001f));
    REQUIRE_THAT(result->z, Catch::Matchers::WithinRel(1.0f, 0.0001f));
}

TEST_CASE("ConfigStore: Set and get string list", "[config][manager]")
{
    ConfigStore config;

    std::vector<std::string> maps = {"map1", "map2", "map3"};
    config.set("game", "maps", maps);
    auto result = config.get<std::vector<std::string>>("game", "maps");

    REQUIRE(result.has_value());
    REQUIRE(result->size() == 3);
    REQUIRE((*result)[0] == "map1");
    REQUIRE((*result)[1] == "map2");
    REQUIRE((*result)[2] == "map3");
}

TEST_CASE("ConfigStore: Get non-existent key", "[config][manager]")
{
    ConfigStore config;

    auto result = config.get<std::string>("nonexistent", "key");

    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::KeyNotFound);
}

TEST_CASE("ConfigStore: Get with wrong type", "[config][manager]")
{
    ConfigStore config;

    config.set("test", "value", std::string("not a number"));
    auto result = config.get<float>("test", "value");

    REQUIRE(!result.has_value());
    REQUIRE(result.error() == ConfigError::ParseError);
}

TEST_CASE("ConfigStore: Update existing value", "[config][manager]")
{
    ConfigStore config;

    config.set("test", "counter", 1);
    config.set("test", "counter", 2);
    config.set("test", "counter", 3);
    auto result = config.get<int32_t>("test", "counter");

    REQUIRE(result.has_value());
    REQUIRE(*result == 3);
}

TEST_CASE("ConfigStore: Save to file", "[config][manager]")
{
    TempConfigFile tempFile("test_save.ini");
    ConfigStore config;

    config.set("graphics", "width", 1920);
    config.set("graphics", "height", 1080);
    config.set("audio", "volume", 0.8f);

    config.save(tempFile.path());

    std::string content = tempFile.read();
    REQUIRE(content.find("[graphics]") != std::string::npos);
    REQUIRE(content.find("width = 1920") != std::string::npos);
    REQUIRE(content.find("height = 1080") != std::string::npos);
    REQUIRE(content.find("[audio]") != std::string::npos);
    REQUIRE(content.find("volume = ") != std::string::npos);
}

TEST_CASE("ConfigStore: Load from file", "[config][manager]")
{
    TempConfigFile tempFile("test_load.ini");
    tempFile.write(
        "[graphics]\n"
        "width = 1920\n"
        "height = 1080\n"
        "vsync = true\n"
        "\n"
        "[audio]\n"
        "volume = 0.75\n"
    );

    ConfigStore config;
    config.load(tempFile.path());

    auto width = config.get<int32_t>("graphics", "width");
    auto height = config.get<int32_t>("graphics", "height");
    auto vsync = config.get<bool>("graphics", "vsync");
    auto volume = config.get<float>("audio", "volume");

    REQUIRE(width.has_value());
    REQUIRE(*width == 1920);
    REQUIRE(height.has_value());
    REQUIRE(*height == 1080);
    REQUIRE(vsync.has_value());
    REQUIRE(*vsync == true);
    REQUIRE(volume.has_value());
    REQUIRE_THAT(*volume, Catch::Matchers::WithinRel(0.75f, 0.01f));
}

TEST_CASE("ConfigStore: Load non-existent file does nothing", "[config][manager]")
{
    ConfigStore config;
    config.set("test", "value", std::string("original"));

    config.load("/nonexistent/path/config.ini");

    auto result = config.get<std::string>("test", "value");
    REQUIRE(result.has_value());
    REQUIRE(*result == "original");
}

TEST_CASE("ConfigStore: Save and load round-trip", "[config][manager]")
{
    TempConfigFile tempFile("test_roundtrip.ini");

    {
        ConfigStore config;
        config.set("graphics", "width", 2560);
        config.set("graphics", "height", 1440);
        config.set("graphics", "vsync", false);
        config.set("audio", "volume", 0.9f);
        config.set("game", "difficulty", std::string("hard"));

        config.save(tempFile.path());
    }

    {
        ConfigStore config;
        config.load(tempFile.path());

        REQUIRE(*config.get<int32_t>("graphics", "width") == 2560);
        REQUIRE(*config.get<int32_t>("graphics", "height") == 1440);
        REQUIRE(*config.get<bool>("graphics", "vsync") == false);
        REQUIRE_THAT(*config.get<float>("audio", "volume"),
                    Catch::Matchers::WithinRel(0.9f, 0.01f));
        REQUIRE(*config.get<std::string>("game", "difficulty") == "hard");
    }
}

TEST_CASE("ConfigStore: Section ordering - unnamed first, vkShade second, then alphabetical", "[config][manager]")
{
    TempConfigFile tempFile("test_ordering.ini");
    ConfigStore config;

    config.set("", "global", std::string("value"));
    config.set("zebra", "last", std::string("z"));
    config.set("vkShade", "app", std::string("setting"));
    config.set("apple", "first", std::string("a"));

    config.save(tempFile.path());

    std::string content = tempFile.read();
    size_t global_pos = content.find("global =");
    size_t vkshade_pos = content.find("[vkShade]");
    size_t apple_pos = content.find("[apple]");
    size_t zebra_pos = content.find("[zebra]");

    REQUIRE(global_pos != std::string::npos);
    REQUIRE(vkshade_pos != std::string::npos);
    REQUIRE(apple_pos != std::string::npos);
    REQUIRE(zebra_pos != std::string::npos);
    REQUIRE(global_pos < vkshade_pos);
    REQUIRE(vkshade_pos < apple_pos);
    REQUIRE(apple_pos < zebra_pos);
}

TEST_CASE("ConfigStore: No trailing newline after last section", "[config][manager]")
{
    TempConfigFile tempFile("test_no_trailing.ini");
    ConfigStore config;

    config.set("section1", "key", std::string("value"));
    config.set("section2", "key", std::string("value"));

    config.save(tempFile.path());

    std::string content = tempFile.read();
    REQUIRE(!content.empty());
    REQUIRE(content.back() == '\n');  // Should end with newline from last key

    // Count occurrences of double newlines (section separators)
    size_t double_newlines = 0;
    for (size_t i = 0; i < content.size() - 1; i++)
    {
        if (content[i] == '\n' && content[i + 1] == '\n')
            double_newlines++;
    }
    REQUIRE(double_newlines == 1);  // Only one separator between two sections
}

TEST_CASE("ConfigStore: Unnamed section appears first", "[config][manager]")
{
    TempConfigFile tempFile("test_unnamed.ini");
    ConfigStore config;

    config.set("", "global_key", std::string("global_value"));
    config.set("vkShade", "app_key", std::string("app_value"));
    config.set("other", "other_key", std::string("other_value"));

    config.save(tempFile.path());

    std::string content = tempFile.read();
    size_t global_pos = content.find("global_key");
    size_t vkshade_pos = content.find("[vkShade]");
    size_t other_pos = content.find("[other]");

    REQUIRE(global_pos != std::string::npos);
    REQUIRE(global_pos < vkshade_pos);
    REQUIRE(vkshade_pos < other_pos);
}

TEST_CASE("ConfigStore: Load with vec3", "[config][manager]")
{
    TempConfigFile tempFile("test_vec3.ini");
    tempFile.write(
        "[graphics]\n"
        "color = 1.0, 0.5, 0.25\n"
    );

    ConfigStore config;
    config.load(tempFile.path());

    auto color = config.get<glm::vec3>("graphics", "color");
    REQUIRE(color.has_value());
    REQUIRE_THAT(color->x, Catch::Matchers::WithinRel(1.0f, 0.0001f));
    REQUIRE_THAT(color->y, Catch::Matchers::WithinRel(0.5f, 0.0001f));
    REQUIRE_THAT(color->z, Catch::Matchers::WithinRel(0.25f, 0.0001f));
}

TEST_CASE("ConfigStore: Load with string list", "[config][manager]")
{
    TempConfigFile tempFile("test_list.ini");
    tempFile.write(
        "[game]\n"
        "maps = map1, map2, map3\n"
    );

    ConfigStore config;
    config.load(tempFile.path());

    auto maps = config.get<std::vector<std::string>>("game", "maps");
    REQUIRE(maps.has_value());
    REQUIRE(maps->size() == 3);
    REQUIRE((*maps)[0] == "map1");
    REQUIRE((*maps)[1] == "map2");
    REQUIRE((*maps)[2] == "map3");
}
