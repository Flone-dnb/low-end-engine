#pragma once

// Standard.
#include <string_view>

/** Editor-global constant values. */
class EditorConstants {
public:
    /**
     * Returns prefix for nodes that should not be displayed in the node tree inspector.
     *
     * @return Prefix for node names.
     */
    static constexpr std::string_view getHiddenNodeNamePrefix() { return "Editor "; }
};
