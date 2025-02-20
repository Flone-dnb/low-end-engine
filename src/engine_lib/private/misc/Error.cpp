#include "misc/Error.h"

// Standard.
#include <string>
#include <filesystem>

// Custom.
#include "io/Logger.h"

// External.
#include "SDL_messagebox.h"

Error::Error(std::string_view sMessage, const std::source_location location) {
    this->sMessage = sMessage;

    stack.push_back(sourceLocationToInfo(location));
}

#if defined(WIN32)
Error::Error(const HRESULT hResult, const std::source_location location) {
    LPSTR pErrorText = nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hResult,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        reinterpret_cast<LPSTR>(&pErrorText),
        0,
        nullptr);

    // Add error code to the beginning of the message.
    std::stringstream hexStream;
    hexStream << std::hex << hResult;
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
    sErrorMessage += sMessage;
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
    const std::string sErrorMessage = getFullErrorMessage();
    Logger::get().error(sErrorMessage);

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
