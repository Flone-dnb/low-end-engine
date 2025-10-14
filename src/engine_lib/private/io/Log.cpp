#include "io/Log.h"

// Standard.
#include <ctime>
#include <fstream>
#include <format>

// Custom.
#include "misc/Globals.h"
#include "misc/ProjectPaths.h"
#if defined(DEBUG)
#include "render/DebugDrawer.h"
#endif

// External.
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#if defined(DEBUG)
namespace {
    constexpr float debugTextHeight = 0.025F;
    constexpr float debugTextTimeSec = 6.0F;
}
#endif

Log& Log::get() {
    static Log logger;
    return logger;
}

LoggerCallbackGuard::~LoggerCallbackGuard() { Log::get().onLogMessage = {}; }

Log::~Log() {
    if (iTotalWarningsProduced.load() > 0 || iTotalErrorsProduced.load() > 0) {
        pSpdLogger->info(std::format(
            "\n---------------------------------------------------\n"
            "Total WARNINGS produced: {}.\n"
            "Total ERRORS produced: {}."
            "\n---------------------------------------------------\n",
            iTotalWarningsProduced.load(),
            iTotalErrorsProduced.load()));
    }

    // Make sure the log is flushed.
    flushToDisk();
}

size_t Log::getTotalWarningsProduced() { return iTotalWarningsProduced.load(); }

size_t Log::getTotalErrorsProduced() { return iTotalErrorsProduced.load(); }

void Log::info(std::string_view sText, const std::source_location location) {
    const auto sMessage = std::format(
        "[{}, {}] {}",
        std::filesystem::path(location.file_name()).filename().string(),
        location.line(),
        sText);

    get().pSpdLogger->info(sMessage);

    if (get().onLogMessage) {
        get().onLogMessage(LogMessageCategory::INFO, sMessage);
    }
}

void Log::warn(std::string_view sText, const std::source_location location) {
    const auto sMessage = std::format(
        "[{}:{}] {}",
        std::filesystem::path(location.file_name()).filename().string(),
        location.line(),
        sText);

    get().pSpdLogger->warn(sMessage);
    iTotalWarningsProduced.fetch_add(1);

    if (get().onLogMessage) {
        get().onLogMessage(LogMessageCategory::WARNING, sMessage);
    }

#if defined(DEBUG) && !defined(ENGINE_EDITOR)
    DebugDrawer::drawText(sMessage, debugTextTimeSec, glm::vec3(1.0F, 1.0F, 0.0F), {}, debugTextHeight);
#endif
}

void Log::error(std::string_view sText, const std::source_location location) {
    const auto sMessage = std::format(
        "[{}:{}] {}",
        std::filesystem::path(location.file_name()).filename().string(),
        location.line(),
        sText);

    get().pSpdLogger->error(sMessage);
    iTotalErrorsProduced.fetch_add(1);

    if (get().onLogMessage) {
        get().onLogMessage(LogMessageCategory::ERROR, sMessage);
    }

#if defined(DEBUG) && !defined(ENGINE_EDITOR)
    DebugDrawer::drawText(sMessage, debugTextTimeSec, glm::vec3(1.0F, 0.0F, 0.0F), {}, debugTextHeight);
#endif
}

void Log::flushToDisk() { get().pSpdLogger->flush(); }

std::unique_ptr<LoggerCallbackGuard>
Log::setCallback(const std::function<void(LogMessageCategory, const std::string&)>& onLogMessage) {
    get().onLogMessage = onLogMessage;

    return std::unique_ptr<LoggerCallbackGuard>(new LoggerCallbackGuard());
}

std::filesystem::path Log::getDirectoryWithLogs() { return get().sLoggerWorkingDirectory; }

Log::Log() {
    auto sLoggerFilePath = ProjectPaths::getPathToLogsDirectory();

    if (!std::filesystem::exists(sLoggerFilePath)) {
        std::filesystem::create_directories(sLoggerFilePath);
    }

    sLoggerWorkingDirectory = sLoggerFilePath;
    sLoggerFilePath /= Globals::getApplicationName() + "-" + getDateTime() + sLogFileExtension;

    removeOldestLogFiles(sLoggerWorkingDirectory);

    if (!std::filesystem::exists(sLoggerFilePath)) {
        std::ofstream logFile(sLoggerFilePath);
        logFile.close();
    }

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(sLoggerFilePath.string(), true);
#if defined(DEBUG)
    // Only write to console if we are in the debug build, there's no need to do this in release builds.
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    pSpdLogger = std::unique_ptr<spdlog::logger>(new spdlog::logger("MainLogger", {consoleSink, fileSink}));
#else
    pSpdLogger = std::unique_ptr<spdlog::logger>(new spdlog::logger("MainLogger", fileSink));
#endif

    // Setup logger.
    pSpdLogger->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    pSpdLogger->flush_on(spdlog::level::warn); // flush log on warnings and errors
}

std::string Log::getDateTime() {
    const time_t now = time(nullptr);

    tm tm{}; // NOLINT
#if defined(WIN32)
    const auto iError = localtime_s(&tm, &now);
    if (iError != 0) {
        get().error(std::format("failed to get localtime (error code {})", iError));
    }
#elif __linux__
    localtime_r(&now, &tm);
#else
    static_assert(false, "not implemented");
#endif

    return std::format("{}.{}_{}-{}-{}", 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void Log::removeOldestLogFiles(const std::filesystem::path& sLogDirectory) {
    size_t iFileCount = 0;

    auto oldestTime = std::chrono::file_clock::now();
    std::filesystem::path oldestFilePath = "";

    for (const auto& entry : std::filesystem::directory_iterator(sLogDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        iFileCount += 1;

        auto fileLastWriteTime = entry.last_write_time();
        if (fileLastWriteTime < oldestTime) {
            oldestTime = fileLastWriteTime;
            oldestFilePath = entry.path();
        }
    }

    if (iFileCount < iMaxLogFiles) {
        return;
    }

    if (oldestFilePath.empty()) {
        return;
    }

    std::filesystem::remove(oldestFilePath);
}
