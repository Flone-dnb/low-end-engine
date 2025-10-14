#pragma once

// Standard.
#include <memory>
#include <filesystem>
#include <source_location>
#include <functional>

// External.
#include "spdlog/spdlog.h"

#if defined(WIN32)
// some windows macro that we use in enum
#undef ERROR
#endif

/** Types of log messages. */
enum class LogMessageCategory : unsigned char { INFO, WARNING, ERROR };

/** RAII-style type that creates logger callback on construction and unregisters it on destruction. */
class LoggerCallbackGuard {
    // Only logger should be able to create objects of this type.
    friend class Log;

public:
    LoggerCallbackGuard(const LoggerCallbackGuard&) = delete;
    LoggerCallbackGuard& operator=(const LoggerCallbackGuard&) = delete;

    ~LoggerCallbackGuard();

private:
    LoggerCallbackGuard() = default;
};

/** Logs to file and console. */
class Log {
    // Clear callback in destructor.
    friend class LoggerCallbackGuard;

public:
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    ~Log();

    /**
     * Returns the total number of warnings produced at this point.
     *
     * @return Warning count.
     */
    static size_t getTotalWarningsProduced();

    /**
     * Returns the total number of errors produced at this point.
     *
     * @return Error count.
     */
    static size_t getTotalErrorsProduced();

    /**
     * Add text to console and log file using "info" category.
     * The text message will be appended with the file name and the line it was called from.
     *
     * @param sText     Text to write to log.
     * @param location  Should not be passed explicitly.
     */
    static void
    info(std::string_view sText, const std::source_location location = std::source_location::current());

    /**
     * Add text to console and log file using "warning" category.
     * The text message will be appended with the file name and the line it was called from.
     *
     * @remark Forces the log to be flushed on the disk.
     *
     * @param sText  Text to write to log.
     * @param location Should not be passed explicitly.
     */
    static void
    warn(std::string_view sText, const std::source_location location = std::source_location::current());

    /**
     * Add text to console and log file using "error" category.
     * The text message will be appended with the file name and the line it was called from.
     *
     * @remark Forces the log to be flushed on the disk.
     *
     * @param sText  Text to write to log.
     * @param location Should not be passed explicitly.
     */
    static void
    error(std::string_view sText, const std::source_location location = std::source_location::current());

    /**
     * Forces the log to be flushed to the disk.
     *
     * @remark Note that you are not required to call this explicitly as the logger will
     * automatically flush the log to the disk from time to time but you can also
     * explicitly call this function when you need to make sure the current log is
     * fully saved on the disk.
     */
    static void flushToDisk();

    /**
     * Sets callback that will be called after a log message is created.
     *
     * @param onLogMessage Callback.
     *
     * @return RAII-style object that will unregister the callback on destruction.
     */
    [[nodiscard]] static std::unique_ptr<LoggerCallbackGuard>
    setCallback(const std::function<void(LogMessageCategory, const std::string&)>& onLogMessage);

    /**
     * Returns the directory that contains all logs.
     *
     * @return Directory for logs.
     */
    static std::filesystem::path getDirectoryWithLogs();

private:
    Log();

    /**
     * Returns current date and time in format "month.day_hour-minute-second".
     *
     * @return Date and time string.
     */
    static std::string getDateTime();

    /**
     * Removes oldest log files if the number of log files exceed a specific limit.
     *
     * @param sLogDirectory Directory that contains log files.
     */
    static void removeOldestLogFiles(const std::filesystem::path& sLogDirectory);

    /**
     * Returns logger singleton.
     *
     * @return Logger.
     */
    static Log& get();

    /** Spdlog logger. */
    std::unique_ptr<spdlog::logger> pSpdLogger = nullptr;

    /** Optional callback that should be called after a log message is created. */
    std::function<void(LogMessageCategory, const std::string&)> onLogMessage;

    /** Directory that is used to create logs. */
    std::filesystem::path sLoggerWorkingDirectory;

    /** The total number of warnings produced. */
    inline static std::atomic<size_t> iTotalWarningsProduced{0};

    /** The total number of errors produced. */
    inline static std::atomic<size_t> iTotalErrorsProduced{0};

    /**
     * The maximum number of log files in the logger directory.
     * If the logger directory contains this amount of log files,
     * the oldest log file will be removed to create a new one.
     */
    inline static constexpr size_t iMaxLogFiles = 5;

    /** Extension of the log files. */
    inline static const char* sLogFileExtension = ".log";
};
