﻿#include "io/Logger.h"

// Standard.
#include <ctime>
#include <fstream>
#include <format>

// Custom.
#include "misc/Globals.h"
#include "misc/ProjectPaths.h"

// External.
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

Logger::~Logger() {
    if (iTotalWarningsProduced.load() > 0 || iTotalErrorsProduced.load() > 0) {
        pSpdLogger->info(
            std::format(
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

Logger& Logger::get() {
    // (some weird error in `last_write_time` code or clang-tidy's false-positive)
    // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
    static Logger logger;
    // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)
    return logger;
}

size_t Logger::getTotalWarningsProduced() { return iTotalWarningsProduced.load(); }

size_t Logger::getTotalErrorsProduced() { return iTotalErrorsProduced.load(); }

void Logger::info(std::string_view sText, const std::source_location location) const {
    pSpdLogger->info(
        std::format(
            "[{}, {}] {}",
            std::filesystem::path(location.file_name()).filename().string(),
            location.line(),
            sText));
}

void Logger::warn(std::string_view sText, const std::source_location location) const {
    pSpdLogger->warn(
        std::format(
            "[{}:{}] {}",
            std::filesystem::path(location.file_name()).filename().string(),
            location.line(),
            sText));
    iTotalWarningsProduced.fetch_add(1);
}

void Logger::error(std::string_view sText, const std::source_location location) const {
    pSpdLogger->error(
        std::format(
            "[{}:{}] {}",
            std::filesystem::path(location.file_name()).filename().string(),
            location.line(),
            sText));
    iTotalErrorsProduced.fetch_add(1);
}

void Logger::flushToDisk() { pSpdLogger->flush(); }

std::filesystem::path Logger::getDirectoryWithLogs() const { return sLoggerWorkingDirectory; }

Logger::Logger() {
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

std::string Logger::getDateTime() {
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

void Logger::removeOldestLogFiles(const std::filesystem::path& sLogDirectory) {
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
