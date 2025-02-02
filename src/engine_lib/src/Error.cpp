#include "Error.h"

// Standard.
#include <string>
#include <filesystem>

Error::Error(std::string_view sMessage, const std::source_location location) {
    this->sMessage = sMessage;

    stack.push_back(sourceLocationToInfo(location));
}

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

void Error::showError() const {
    throw std::runtime_error(getFullErrorMessage());
    //     const std::string sErrorMessage = getFullErrorMessage();
    //     Logger::get().error(sErrorMessage);

    // #if defined(WIN32)
    // #pragma push_macro("MessageBox")
    // #undef MessageBox
    // #endif
    //     MessageBox::error("Error", sErrorMessage);
    // #if defined(WIN32)
    // #pragma pop_macro("MessageBox")
    // #endif
}

SourceLocationInfo Error::sourceLocationToInfo(const std::source_location& location) {
    SourceLocationInfo info;
    info.sFilename = std::filesystem::path(location.file_name()).filename().string();
    info.sLine = std::to_string(location.line());

    return info;
}
