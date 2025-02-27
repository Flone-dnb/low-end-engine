﻿#pragma once

// Standard.
#include <memory>
#include <filesystem>
#include <source_location>

// External.
#include "spdlog/spdlog.h"

/**
 * Logs to file and console.
 */
class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    ~Logger();

    /**
     * Returns a reference to the logger instance.
     * If no instance was created yet, this function will create it
     * and return a reference to it.
     *
     * @return Reference to the logger instance.
     */
    static Logger& get();

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
    void info(
        std::string_view sText,
        const std::source_location location = std::source_location::current()) const; // NOLINT

    /**
     * Add text to console and log file using "warning" category.
     * The text message will be appended with the file name and the line it was called from.
     *
     * @remark Forces the log to be flushed on the disk.
     *
     * @param sText  Text to write to log.
     * @param location Should not be passed explicitly.
     */
    void warn(
        std::string_view sText,
        const std::source_location location = std::source_location::current()) const; // NOLINT

    /**
     * Add text to console and log file using "error" category.
     * The text message will be appended with the file name and the line it was called from.
     *
     * @remark Forces the log to be flushed on the disk.
     *
     * @param sText  Text to write to log.
     * @param location Should not be passed explicitly.
     */
    void error(
        std::string_view sText,
        const std::source_location location = std::source_location::current()) const; // NOLINT

    /**
     * Forces the log to be flushed to the disk.
     *
     * @remark Note that you are not required to call this explicitly as the logger will
     * automatically flush the log to the disk from time to time but you can also
     * explicitly call this function when you need to make sure the current log is
     * fully saved on the disk.
     */
    void flushToDisk();

    /**
     * Returns the directory that contains all logs.
     *
     * @return Directory for logs.
     */
    std::filesystem::path getDirectoryWithLogs() const;

private:
    Logger();

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

    /** Spdlog logger. */
    std::unique_ptr<spdlog::logger> pSpdLogger = nullptr;

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
