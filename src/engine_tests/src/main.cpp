#if defined(WIN32)
#include <Windows.h>
#include <crtdbg.h>
#endif

// Standard.
#include <filesystem>

// Custom.
#include "io/Logger.h"
#include "TestFilePaths.hpp"
#include "misc/ProjectPaths.h"

// External.
#include "catch2/catch_session.hpp"
#include "catch2/catch_test_case_info.hpp"
#include "catch2/reporters/catch_reporter_event_listener.hpp"
#include "catch2/reporters/catch_reporter_registrars.hpp"

struct EventListener : Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        Logger::get().info(std::format("\n\nstarting test case \"{}\"\n", testInfo.name));
        Logger::get().flushToDisk();
    }

    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        if (testCaseStats.totals.assertions.failed > 0) {
            std::scoped_lock guard(mtxFailedTestNames.first);
            mtxFailedTestNames.second.push_back(testCaseStats.testInfo->name);
        }
    }

    void testRunEnded(Catch::TestRunStats const& runStats) override {
        std::scoped_lock guard(mtxFailedTestNames.first);

        if (mtxFailedTestNames.second.empty()) {
            return;
        }

        Logger::get().info("failed tests:");
        for (const auto& sName : mtxFailedTestNames.second) {
            Logger::get().info(std::format("- {}", sName));
        }
    }

private:
    std::pair<std::mutex, std::vector<std::string>> mtxFailedTestNames;
};

CATCH_REGISTER_LISTENER(EventListener)

int main() {
    // Enable run-time memory check for debug builds (on Windows).
#if defined(WIN32) && defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#elif defined(WIN32) && !defined(DEBUG)
    OutputDebugStringA("Using release build configuration, memory checks are disabled.");
#endif

    // Delete old temp files.
    const auto pathToTempDir = ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName;
    if (std::filesystem::exists(pathToTempDir)) {
        std::filesystem::remove_all(pathToTempDir);
    }
    std::filesystem::create_directory(pathToTempDir);

    const int iResult = Catch::Session().run();
    if (iResult != 0) {
        return iResult;
    }

    // Cleanup temp files.
    std::filesystem::remove_all(pathToTempDir);

    return 0;
}
