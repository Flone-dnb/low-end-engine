#include "misc/Globals.h"

// Standard.
#include <stdexcept>
#include <filesystem>

// Custom.
#include "misc/Error.h"
#if defined(WIN32)
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#elif __linux__
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <cstddef>
#include <cstdlib>
#include <wchar.h>
#include <stdio.h>
#include <limits.h>
#endif

std::string Globals::getApplicationName() {
#if defined(WIN32)

    TCHAR buffer[MAX_PATH] = {0};
    constexpr auto bufSize = static_cast<DWORD>(std::size(buffer));

    // Get the fully-qualified path of the executable.
    if (GetModuleFileName(nullptr, buffer, bufSize) == bufSize) {
        Error::showErrorAndThrowException("failed to get path to the application, path is too long");
    }

    return std::filesystem::path(buffer).stem().string();

#elif __linux__

    constexpr size_t bufSize = 1024; // NOLINT: should be more that enough
    char buf[bufSize] = {0};
    if (readlink("/proc/self/exe", &buf[0], bufSize) == -1) {
        Error::showErrorAndThrowException("failed to get path to the application");
    }

    return std::filesystem::path(buf).stem().string();

#else

    static_assert(false, "not implemented");

#endif
}

std::string Globals::wstringToString(const std::wstring& sText) {
    std::string sOutput;

    sOutput.resize(sText.length());

#if defined(WIN32)
    size_t iResultBytes;
    wcstombs_s(&iResultBytes, sOutput.data(), sOutput.size() + 1, sText.c_str(), sText.size());
#elif __linux__
    static std::mutex mtxWcstombs;
    {
        std::scoped_lock guard(mtxWcstombs);
        wcstombs(sOutput.data(), sText.c_str(), sOutput.size()); // NOLINT: not thread safe
    }
#else
    static_assert(false, "not implemented");
#endif

    return sOutput;
}

std::wstring Globals::stringToWstring(const std::string& sText) {
    std::wstring sOutput;

    sOutput.resize(sText.length());

#if defined(WIN32)
    size_t iResultBytes;
    mbstowcs_s(&iResultBytes, sOutput.data(), sOutput.size() + 1, sText.c_str(), sText.size());
#elif __linux__
    mbstowcs(sOutput.data(), sText.c_str(), sOutput.size());
#else
    static_assert(false, "not implemented");
#endif

    return sOutput;
}

std::filesystem::path Globals::getProcessWorkingDirectory() {
    std::filesystem::path pathToWorkingDirectory;

#if defined(WIN32)
    char buffer[MAX_PATH];
    DWORD length = GetCurrentDirectory(MAX_PATH, buffer);
    if (length != 0) {
        pathToWorkingDirectory = buffer;
    } else [[unlikely]] {
        Error::showErrorAndThrowException("failed to get path to the working directory of the process");
    }
#elif __linux__
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        pathToWorkingDirectory = cwd;
    } else [[unlikely]] {
        Error::showErrorAndThrowException("failed to get path to the working directory of the process");
    }
#else
    static_assert(false, "not implemented");
#endif

    return pathToWorkingDirectory;
}

std::string Globals::getDebugOnlyLoggingPrefix() { return std::string(sDebugOnlyLoggingPrefix); }

std::string Globals::getResourcesDirectoryName() { return std::string(sResDirectoryName); }

std::string Globals::getEngineDirectoryName() { return std::string(sBaseEngineDirectoryName); }
