﻿#include "misc/ProjectPaths.h"

// Standard.
#include <format>
#if __linux__
#include <mutex>
#endif

// Custom.
#include "misc/Globals.h"
#include "misc/Error.h"

// OS.
#if defined(WIN32)
#include <ShlObj.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#elif __linux__
#include <unistd.h>
#include <sys/types.h>
#include <cstddef>
#include <cstdlib>
#endif

std::filesystem::path ProjectPaths::getPathToGameDirectory() {
#if defined(WIN32)

    TCHAR buffer[MAX_PATH] = {0};
    constexpr auto bufSize = static_cast<DWORD>(std::size(buffer));

    // Get the fully-qualified path of the executable.
    if (GetModuleFileName(nullptr, buffer, bufSize) == bufSize) {
        Error::showErrorAndThrowException("failed to get path to the application, path is too long");
    }

    return std::filesystem::path(buffer).parent_path();

#elif __linux__

    constexpr size_t bufSize = 1024; // NOLINT: should be more that enough
    char buf[bufSize] = {0};
    if (readlink("/proc/self/exe", &buf[0], bufSize) == -1) {
        Error::showErrorAndThrowException("failed to get path to the application");
    }

    return std::filesystem::path(buf).parent_path();

#else

    static_assert(false, "not implemented");

#endif
}

std::filesystem::path ProjectPaths::getPathToEngineConfigsDirectory() {
    return getPathToBaseConfigDirectory() / Globals::getApplicationName() / sEngineDirectoryName;
}

std::filesystem::path ProjectPaths::getPathToLogsDirectory() {
    return getPathToBaseConfigDirectory() / Globals::getApplicationName() / sLogsDirectoryName;
}

std::filesystem::path ProjectPaths::getPathToPlayerProgressDirectory() {
    return getPathToBaseConfigDirectory() / Globals::getApplicationName() / sProgressDirectoryName;
}

std::filesystem::path ProjectPaths::getPathToPlayerSettingsDirectory() {
    return getPathToBaseConfigDirectory() / Globals::getApplicationName() / sSettingsDirectoryName;
}

std::filesystem::path
ProjectPaths::getPathToResDirectory(ResourceDirectory directory, bool bCreateIfNotExists) {
    std::filesystem::path path = getPathToResDirectory();

    switch (directory) {
    case ResourceDirectory::ROOT:
        return path;
    case ResourceDirectory::GAME: {
        path /= sGameResourcesDirectoryName;
        break;
    }
    case ResourceDirectory::ENGINE: {
        path /= sEngineResourcesDirectoryName;
        break;
    }
    case ResourceDirectory::EDITOR: {
        path /= sEditorResourcesDirectoryName;
        break;
    }
    };

    if (!std::filesystem::exists(path)) {
        if (bCreateIfNotExists) {
            std::filesystem::create_directory(path);
        } else {
            Error::showErrorAndThrowException(
                std::format("expected directory \"{}\" to exist", path.string()));
        }
    }

    return path;
}

std::filesystem::path ProjectPaths::getPathToBaseConfigDirectory() {
    std::filesystem::path basePath;

#if defined(WIN32)

    // Try to get AppData folder.
    PWSTR pPathTmp = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pPathTmp);
    if (result != S_OK) {
        CoTaskMemFree(pPathTmp);

        const Error err(result);
        err.showErrorAndThrowException();
    }

    basePath = pPathTmp;

    CoTaskMemFree(pPathTmp);

#elif __linux__

#if defined(__aarch64__)
    // On handheld linux devices I've decided to store configs and logs near the binary so that it will be
    // easier to find them.
    basePath = getPathToGameDirectory();
#else
    static std::mutex mtxGetenv;
    {
        std::scoped_lock guard(mtxGetenv);

        const auto sHomePath = std::string(getenv("HOME")); // NOLINT: not thread safe
        if (sHomePath.empty()) {
            Error::showErrorAndThrowException("environment variable HOME is not set");
        }

        basePath = std::format("{}/.config/", sHomePath);
    }
#endif

#else
    static_assert(false, "not implemented");
#endif

    // Append engine directory.
    basePath /= Globals::getEngineDirectoryName();

    // Create path if not exists.
    if (!std::filesystem::exists(basePath)) {
        std::filesystem::create_directories(basePath);
    }

    return basePath;
}

std::filesystem::path ProjectPaths::getPathToResDirectory() {
    std::filesystem::path pathToExecutable;

#if defined(WIN32)

    TCHAR buffer[MAX_PATH] = {0};
    constexpr auto bufSize = static_cast<DWORD>(std::size(buffer));

    // Get the fully-qualified path of the executable.
    if (GetModuleFileName(nullptr, buffer, bufSize) == bufSize) {
        Error::showErrorAndThrowException("failed to get path to the application, path is too long");
    }

    pathToExecutable = std::filesystem::path(buffer);

#elif __linux__

    constexpr size_t bufSize = 1024; // NOLINT: should be more that enough
    char buf[bufSize] = {0};
    if (readlink("/proc/self/exe", &buf[0], bufSize) == -1) {
        Error::showErrorAndThrowException("failed to get path to the application");
    }

    pathToExecutable = std::filesystem::path(buf);

#else
    static_assert(false, "not implemented");
#endif

    // Construct path to the resources directory.
    auto pathToRes = pathToExecutable.parent_path() / Globals::getResourcesDirectoryName();
    if (!std::filesystem::exists(pathToRes)) {
        Error::showErrorAndThrowException(
            std::format("expected resources directory to exist at \"{}\"", pathToRes.string()));
    }

    return pathToRes;
}
