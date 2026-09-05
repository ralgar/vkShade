#include <catch2/catch_test_macros.hpp>

#include <fstream>

#include "platform/linux/file_watcher.hpp"

using namespace vkShade::Platform;
namespace fs = std::filesystem;

namespace
{
    // Unique temp dir per test case so tests can't interfere with each other.
    fs::path make_temp_dir(const std::string& name)
    {
        fs::path dir = fs::temp_directory_path() / ("vkshade_fw_test_" + name);
        fs::remove_all(dir);
        fs::create_directories(dir);
        return dir;
    }

    void write_file(const fs::path& path, const std::string& content = "x")
    {
        std::ofstream f(path, std::ios::trunc);
        f << content;
    }
}

TEST_CASE("LinuxFileWatcher: watch() succeeds on a valid existing file", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("valid");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));
}

TEST_CASE("LinuxFileWatcher: watch() fails when the directory doesn't exist", "[platform][filewatcher]")
{
    fs::path missing = fs::temp_directory_path() / "vkshade_fw_test_does_not_exist" / "config.ini";

    LinuxFileWatcher watcher;
    REQUIRE_FALSE(watcher.watch(missing));
}

TEST_CASE("LinuxFileWatcher: changed() is false with nothing touched", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("idle");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    REQUIRE_FALSE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Writing to the watched file triggers changed()", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("write");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    write_file(file, "y");

    REQUIRE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Only the watched filename triggers changed()", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("filter");
    fs::path watched = dir / "config.ini";
    fs::path other = dir / "unrelated.ini";
    write_file(watched);
    write_file(other);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(watched));

    write_file(other, "changed");

    // Sibling file in the same watched directory must NOT count as a change.
    REQUIRE_FALSE(watcher.changed());

    // Positive control: Prove the watcher is actually alive and working,
    //  not just silently dead (which would also make the above pass).
    write_file(watched, "changed");
    REQUIRE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Re-watching a new file stops watching the old one", "[platform][filewatcher]")
{
    fs::path dirA = make_temp_dir("rewatch_a");
    fs::path dirB = make_temp_dir("rewatch_b");
    fs::path fileA = dirA / "config.ini";
    fs::path fileB = dirB / "config.ini";
    write_file(fileA);
    write_file(fileB);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(fileA));
    REQUIRE(watcher.watch(fileB));  // Should replace, not add to, the fileA watch

    write_file(fileA, "changed");   // Old watch target — must be silently ignored now

    REQUIRE_FALSE(watcher.changed());

    // Positive control: Prove the watcher is actually alive and working,
    //  not just silently dead (which would also make the above pass).
    write_file(fileB, "changed");
    REQUIRE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Re-watching a new file still detects changes to it", "[platform][filewatcher]")
{
    fs::path dirA = make_temp_dir("rewatch_still_works_a");
    fs::path dirB = make_temp_dir("rewatch_still_works_b");
    fs::path fileA = dirA / "config.ini";
    fs::path fileB = dirB / "config.ini";
    write_file(fileA);
    write_file(fileB);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(fileA));
    REQUIRE(watcher.watch(fileB));

    write_file(fileB, "changed");

    REQUIRE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Deleting the watched directory does not report changed()", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("delete_self");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    fs::remove_all(dir);

    // Per the current design, IN_IGNORED (watch invalidated) is swallowed
    // and reported as changed == false, not true — this locks that behavior
    // in as a regression test.
    REQUIRE_FALSE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Can re-watch after the underlying directory was deleted", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("recover");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    fs::remove_all(dir);
    (void)watcher.changed(); // Drain/process the IN_IGNORED event

    // Recreate and re-watch — confirms m_watchDescriptor was properly reset
    // to -1 and watch() can recover rather than staying wedged.
    fs::create_directories(dir);
    write_file(file);
    REQUIRE(watcher.watch(file));

    write_file(file, "changed");
    REQUIRE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: unwatch() stops detecting changes", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("unwatch_stops");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    watcher.unwatch();

    write_file(file, "changed");

    REQUIRE_FALSE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: unwatch() with no active watch is safe", "[platform][filewatcher]")
{
    LinuxFileWatcher watcher;

    // Never watched anything — should not crash or misbehave.
    watcher.unwatch();
    REQUIRE_FALSE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: unwatch() can be called twice in a row safely", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("unwatch_twice");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    watcher.unwatch();
    watcher.unwatch();  // Second call must be a safe no-op, not UB/crash

    write_file(file, "changed");
    REQUIRE_FALSE(watcher.changed());
}

TEST_CASE("LinuxFileWatcher: Can watch again after unwatch()", "[platform][filewatcher]")
{
    fs::path dir = make_temp_dir("unwatch_then_rewatch");
    fs::path file = dir / "config.ini";
    write_file(file);

    LinuxFileWatcher watcher;
    REQUIRE(watcher.watch(file));

    watcher.unwatch();

    REQUIRE(watcher.watch(file));

    write_file(file, "changed");
    REQUIRE(watcher.changed());
}
