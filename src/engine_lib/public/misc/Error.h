#pragma once

// Standard.
#include <string_view>
#include <vector>
#include <string>
#include <source_location>

/// A handy macro for calling an OpenGL function and checking the last error.
void checkLastGlError(const std::source_location location = std::source_location::current());
#define GL_CHECK_ERROR(a)                                                                                    \
    a;                                                                                                       \
    checkLastGlError();

/** Information of a specific source code location. */
struct SourceLocationInfo {
    /** File name. */
    std::string sFilename;

    /** Line number. */
    std::string sLine;
};

/** Helper class for storing and showing error messages. */
class Error {
public:
    /**
     * Constructs a new Error object.
     *
     * @param sMessage  Error message to show.
     * @param location  Should not be specified explicitly (use default value).
     */
    Error(
        std::string_view sMessage,
        const std::source_location location = std::source_location::current()); // NOLINT

#if defined(WIN32)
    /**
     * Constructs a new Error object from `HRESULT`.
     *
     * @param iResult   `HRESULT` that contains an error.
     * @param location  Should not be specified explicitly (use default value).
     */
    Error(const long iResult, const std::source_location location = std::source_location::current());
#endif

    Error() = delete;
    virtual ~Error() = default;

    /**
     * Copy constructor.
     *
     * @param other other object.
     */
    Error(const Error& other) = default;

    /**
     * Copy assignment.
     *
     * @param other other object.
     *
     * @return Result of copy assignment.
     */
    Error& operator=(const Error& other) = default;

    /**
     * Move constructor.
     *
     * @param other other object.
     */
    Error(Error&& other) = default;

    /**
     * Move assignment.
     *
     * @param other other object.
     *
     * @return Result of move assignment.
     */
    Error& operator=(Error&& other) = default;

    /**
     * Adds the caller's file and line as a new entry to the error location stack.
     *
     * @param location Should not be specified explicitly (use default value).
     */
    void addCurrentLocationToErrorStack(
        const std::source_location location = std::source_location::current()); // NOLINT

    /**
     * Creates an error string that contains an error message and an error location stack.
     *
     * @return Error message and error stack.
     */
    std::string getFullErrorMessage() const;

    /**
     * Returns initial error message that was used to create this error.
     *
     * @return Initial error message.
     */
    std::string getInitialMessage() const;

    /** Logs @ref getFullErrorMessage, shows it on screen and throws an exception. */
    [[noreturn]] void showErrorAndThrowException() const;

protected:
    /**
     * Converts source_location instance to location information.
     *
     * @param location Source location instance.
     *
     * @return Location information.
     */
    static SourceLocationInfo sourceLocationToInfo(const std::source_location& location);

private:
    /** Initial error message (string version). */
    std::string sMessage;

    /** Error stack. */
    std::vector<SourceLocationInfo> stack;
};
