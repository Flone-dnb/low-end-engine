#include "misc/Error.h"

// Standard.
#include <string>
#include <filesystem>

// Custom.
#include "io/Log.h"
#include "misc/MemoryUsage.hpp"

// External.
#include "SDL3/SDL_messagebox.h"
#include "glad/glad.h"
#if defined(WIN32)
#include <Windows.h>
#endif
#if defined(DEBUG)
#include "cpptrace/cpptrace.hpp"
#endif

void checkLastGlError(const std::source_location location) {
    const auto lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        const auto filename = std::filesystem::path(location.file_name()).filename().string();
        std::string sErrorMessage;
        switch (lastError) {
        case GL_INVALID_ENUM:
            sErrorMessage = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            sErrorMessage = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            sErrorMessage = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            sErrorMessage = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            sErrorMessage = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        default:
            sErrorMessage = std::to_string(lastError);
            break;
        }
        Error::showErrorAndThrowException(std::format(
            "an OpenGL error occurred at {}, line {}, error: {}", filename, location.line(), sErrorMessage));
    }
}

Error::Error(std::string_view sMessage, const std::source_location location) {
    // Mark RAM usage.
    const auto iRamTotalMb = MemoryUsage::getTotalMemorySize() / 1024 / 1024;
    const auto iRamUsedMb = MemoryUsage::getTotalMemorySizeUsed() / 1024 / 1024;
    const auto iAppRamMb = MemoryUsage::getMemorySizeUsedByProcess() / 1024 / 1024;

    sRamUsageString = std::format("\n\nRAM (MB): {} ({}/{})\n", iAppRamMb, iRamUsedMb, iRamTotalMb);
    this->sMessage = sMessage;

    stack.push_back(sourceLocationToInfo(location));
}

void Error::showErrorAndThrowException(std::string_view sMessage, const std::source_location location) {
    Error error(sMessage, location);
    error.showErrorAndThrowException();
}

#if defined(WIN32)
Error::Error(const HRESULT iResult, const std::source_location location) {
    LPSTR pErrorText = nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        iResult,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        reinterpret_cast<LPSTR>(&pErrorText),
        0,
        nullptr);

    // Add error code to the beginning of the message.
    std::stringstream hexStream;
    hexStream << std::hex << iResult;
    sMessage = std::format("0x{}: ", hexStream.str());

    if (pErrorText != nullptr) {
        sMessage += std::string_view(pErrorText);
        LocalFree(pErrorText);
    } else {
        sMessage += "unknown error";
    }

    stack.push_back(sourceLocationToInfo(location));
}
#endif

void Error::addCurrentLocationToErrorStack(const std::source_location location) {
    stack.push_back(sourceLocationToInfo(location));
}

std::string Error::getFullErrorMessage() const {
    std::string sErrorMessage = "An error occurred: ";
    sErrorMessage += sMessage + "\n" + sRamUsageString;
    sErrorMessage += "\nError stack:\n";

    for (const auto& entry : stack) {
        sErrorMessage += "- at ";
        sErrorMessage += entry.sFilename;
        sErrorMessage += ", ";
        sErrorMessage += entry.sLine;
        sErrorMessage += "\n";
    }

    return sErrorMessage;
}

std::string Error::getInitialMessage() const { return sMessage; }

void Error::showErrorAndThrowException() const {
    // Log error.
    {
        auto guard = Log::setCallback(nullptr); // unbind callback to not trigger any game/editor logic
    }
    std::string sErrorMessage = getFullErrorMessage();

#if defined(DEBUG)
    sErrorMessage += "\nstacktrace:\n";
    auto stacktrace = cpptrace::generate_trace();
    for (const auto& frame : stacktrace.frames) {
        if (frame.line.has_value()) {
            sErrorMessage += std::format(
                "- {}, line: {}\n",
                std::filesystem::path(frame.filename).filename().string(),
                frame.line.value());
        } else {
            sErrorMessage += std::format("- {}\n", frame.filename);
        }
    }
#endif

    Log::error(sErrorMessage);

    // Show message box.
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", sErrorMessage.c_str(), nullptr);

    // Throw.
    throw std::runtime_error(sErrorMessage);
}

SourceLocationInfo Error::sourceLocationToInfo(const std::source_location& location) {
    SourceLocationInfo info;
    info.sFilename = std::filesystem::path(location.file_name()).filename().string();
    info.sLine = std::to_string(location.line());

    return info;
}
