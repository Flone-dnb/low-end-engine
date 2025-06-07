// This is a small program to run as a post build step to check that all Node derived types in override
// functions are "calling super" (base class implementation of the function being overridden). So that the
// programmer does not need to remember that.

// Standard.
#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string_view>

// External.
#if defined(WIN32)
#include <Windows.h>
#include <crtdbg.h>
#endif

// Helper function for consistent log messages.
void logLine(std::string_view sText) {
    std::cout << std::format("[{}] {}\n", NODE_SUPER_CALL_CHECKER_NAME, sText);
}

int checkClass(const std::filesystem::path& pathToHeaderFile, const std::filesystem::path& pathToCppFile) {
    // Skip base class.
    if (pathToCppFile.filename() == "Node.cpp") {
        return 0;
    }

    bool bFoundGetTypeGuidOverride = false;
    const std::string_view sGetTypeNameFunction = "getTypeGuid";

    bool bFoundOverrideDestructor = false;

    // Get parent class name and override function names from the header file.
    const auto sClassName = pathToHeaderFile.stem().string();
    std::string sParentClassName;
    std::vector<std::string> vOverrideFunctionNames;

    {
        // Open header file.
        std::ifstream file(pathToHeaderFile);
        if (!file.is_open()) [[unlikely]] {
            logLine(std::format("unable to open file \"{}\"", pathToHeaderFile.string()));
            return 1;
        }

        // Read file.
        std::string sCode;
        {
            std::string sCodeLine;
            while (getline(file, sCodeLine)) {
                sCode += sCodeLine;
            }
        }

        // Find parent class name.
        {
            const auto sClassDefinitionText = std::format("class {} : public ", sClassName);
            const auto iClassNamePos = sCode.find(sClassDefinitionText);
            if (iClassNamePos == std::string::npos) [[unlikely]] {
                logLine(std::format(
                    "in the file \"{}\" expected to find a class with the name \"{}\"",
                    pathToHeaderFile.filename().string(),
                    sClassName));
                return 1;
            }

            // Read parent class name.
            for (size_t i = iClassNamePos + sClassDefinitionText.size(); i < sCode.size(); i++) {
                if (sCode[i] == ' ' || sCode[i] == '{') {
                    break;
                }
                sParentClassName += sCode[i];
            }
        }
        if (sParentClassName.empty()) [[unlikely]] {
            logLine(std::format(
                "unable to parse parent class name in the file \"{}\"",
                pathToHeaderFile.filename().string()));
            return 1;
        }

        // Collect all `override` function names.
        size_t iOverridePos = 0;
        do {
            iOverridePos = sCode.find("override", iOverridePos);
            if (iOverridePos == std::string::npos) {
                break;
            }
            iOverridePos += 1;

            // Go back until '('.
            const auto iNameEndPos = sCode.rfind('(', iOverridePos);
            if (iNameEndPos == std::string::npos) [[unlikely]] {
                logLine(std::format(
                    "expected to find `(` before `override` keyword in file \"{}\"",
                    pathToHeaderFile.filename().string()));
                return 1;
            }

            // Go back until a space is found.
            size_t iNameStartPos = 0;
            bool bIsDestructor = false;
            for (size_t i = iNameEndPos - 1; i > 0; i--) {
                if (sCode[i] == ' ') {
                    iNameStartPos = i + 1;
                    break;
                } else if (sCode[i] == '~') {
                    bIsDestructor = true;
                    break;
                }
            }
            if (bIsDestructor) {
                bFoundOverrideDestructor = true;
                continue;
            }
            if (iNameStartPos == 0) [[unlikely]] {
                logLine(std::format(
                    "expected to find ` ` (space) before override function name in file \"{}\"",
                    pathToHeaderFile.filename().string()));
                return 1;
            }

            // Cut function name.
            const auto sFunctionName = sCode.substr(iNameStartPos, iNameEndPos - iNameStartPos);

            if (sFunctionName == sGetTypeNameFunction) {
                bFoundGetTypeGuidOverride = true;
                // Don't need to call super in this function.
                continue;
            }

            vOverrideFunctionNames.push_back(sFunctionName);
        } while (iOverridePos != std::string::npos);
    }

    if (!bFoundOverrideDestructor) [[unlikely]] {
        logLine(std::format("you need to override destructor for your node \"{}\"", sClassName));
        return 1;
    }
    if (!bFoundGetTypeGuidOverride) [[unlikely]] {
        logLine(std::format(
            "you need to override the function \"{}\" in \"{}\"", sGetTypeNameFunction, sClassName));
        return 1;
    }

    // Now check .cpp file.
    {
        // Open header file.
        std::ifstream file(pathToCppFile);
        if (!file.is_open()) [[unlikely]] {
            logLine(std::format("unable to open file \"{}\"", pathToCppFile.string()));
            return 1;
        }

        // Read file.
        std::string sCode;
        {
            std::string sCodeLine;
            while (getline(file, sCodeLine)) {
                sCode += sCodeLine;
            }
        }

        // Check every override function.
        for (const auto& sOverrideFunctionName : vOverrideFunctionNames) {
            // Find this function.
            const auto sOverrideFuncText = std::format("{}::{}(", sClassName, sOverrideFunctionName);
            const auto iOverridePos = sCode.find(sOverrideFuncText);
            if (iOverridePos == std::string::npos) [[unlikely]] {
                logLine(std::format(
                    "unable to find \"{}\" in the file \"{}\"",
                    sOverrideFuncText,
                    pathToCppFile.filename().string()));
                return 1;
            }

            // Find `{` after the function name.
            const auto iImplStartPos = sCode.find('{', iOverridePos + sOverrideFuncText.size());
            if (iImplStartPos == std::string::npos) [[unlikely]] {
                logLine(std::format(
                    "unable to find \"{{\" somewhere after \"{}\" in the file \"{}\"",
                    sOverrideFuncText,
                    pathToCppFile.filename().string()));
                return 1;
            }

            // Find super call somewhere in the implementation.
            const auto sSuperCallText = std::format("{}::{}(", sParentClassName, sOverrideFunctionName);
            size_t iNestingCount = 0;
            bool bFoundSuperCall = false;
            for (size_t i = iImplStartPos + 1; i < sCode.size(); i++) {
                if (sCode[i] == '}') {
                    if (iNestingCount == 0) {
                        // End of the function.
                        break;
                    }
                    iNestingCount -= 1;
                } else if (sCode[i] == '{') {
                    iNestingCount += 1;
                } else if (sCode[i] == sSuperCallText[0]) {
                    if (std::string_view{sCode.data() + i, sSuperCallText.size()} == sSuperCallText) {
                        bFoundSuperCall = true;
                    }
                }
            }
            if (!bFoundSuperCall) [[unlikely]] {
                logLine(std::format(
                    "file \"{}\", function \"{}\": expected to find a call to the parent's "
                    "implementation "
                    "like so: \"{}\"",
                    pathToCppFile.filename().string(),
                    sOverrideFunctionName,
                    sSuperCallText));
                return 1;
            }
        }
    }

    return 0;
}

int checkFiles(
    const std::vector<std::filesystem::path>& vPathsToCppFiles,
    const std::unordered_map<std::string, std::filesystem::path>& headerFileStemToPath) {
    // For each .cpp file find according .h/.hpp file and process them.
    for (const auto& pathToCppFile : vPathsToCppFiles) {
        const auto headerIt = headerFileStemToPath.find(pathToCppFile.stem().string());
        if (headerIt == headerFileStemToPath.end()) [[unlikely]] {
            logLine(std::format(
                "unable to find a header file for the .cpp file \"{}\"", pathToCppFile.filename().string()));
            return 1;
        }

        if (checkClass(headerIt->second, pathToCppFile) != 0) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    // Enable run-time memory check for debug builds (on Windows).
#if defined(WIN32) && defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#elif defined(WIN32) && !defined(DEBUG)
    OutputDebugStringA("Using release build configuration, memory checks are disabled.");
#endif

    // Mark start time.
    logLine("starting...");
    const auto startTime = std::chrono::steady_clock::now();

    // Expecting 2 arguments:
    // - global path to the directory with node .cpp files to check
    // - global path to the directory with node .h files to check
    if (argc != 3) [[unlikely]] {
        logLine(std::format("expected 2 arguments, received {}:", argc - 1));
        for (int i = 1; i < argc; i++) {
            logLine(std::format("{}. {}", i, argv[i]));
        }
        return 1;
    }
    const std::filesystem::path pathToDirectory1 = argv[1];
    const std::filesystem::path pathToDirectory2 = argv[2];

    // Gather directories to check.
    std::vector<std::filesystem::path> vPathsToDirectoryToCheck;
    vPathsToDirectoryToCheck.push_back(pathToDirectory1);
    if (pathToDirectory1 != pathToDirectory2) {
        vPathsToDirectoryToCheck.push_back(pathToDirectory2);
    }

    std::vector<std::filesystem::path> vCppFilePaths;
    std::unordered_map<std::string, std::filesystem::path> headerFileStemToPath;

    for (const auto& pathToDirectory : vPathsToDirectoryToCheck) {
        // Make sure it's a directory.
        if (!std::filesystem::is_directory(pathToDirectory)) [[unlikely]] {
            logLine(std::format("expected the path \"{}\" to be a directory", pathToDirectory.string()));
            return 1;
        }

        // Collect files.
        for (const auto& entry : std::filesystem::recursive_directory_iterator(pathToDirectory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (!entry.path().has_extension()) {
                continue;
            }

            const auto fileExtension = entry.path().extension();
            if (fileExtension == ".h" || fileExtension == ".hpp") {
                const auto sFileStem = entry.path().stem().string();
                const auto [it, bIsAddedNew] = headerFileStemToPath.insert({sFileStem, entry.path()});
                if (!bIsAddedNew) [[unlikely]] {
                    logLine(std::format("found 2 files with the same name \"{}\"", sFileStem));
                    return 1;
                }
            } else if (fileExtension == ".cpp") {
                vCppFilePaths.push_back(entry.path());
            } else [[unlikely]] {
                logLine(std::format(
                    "unexpected file extension for file \"{}\"", entry.path().filename().string()));
                return 1;
            }
        }
    }

    if (checkFiles(vCppFilePaths, headerFileStemToPath) != 0) {
        return 1;
    }

    // Mark end time.
    const auto endTime = std::chrono::steady_clock::now();
    const auto timeTookInMs =
        std::chrono::duration<float, std::chrono::milliseconds::period>(endTime - startTime).count();

    logLine(std::format("finished, took {:.1F} ms", timeTookInMs));

    return 0;
}
