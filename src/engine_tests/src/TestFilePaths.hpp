#pragma once

#include <array>
#include <string_view>

static constexpr std::string_view sTestDirName = "test";

static constexpr std::array<std::string_view, 13> vUsedTestFileNames = {
    "serializable",
    "serializable_derived",
    "node_tree",
    "parent_node_tree",
    "external_node_tree",
    "2serializable_original",
    "modified_original",
    "modified_same_file",
    "2mesh_nodes",
    "load_node_tree_as_world",
    "layout_ui",
    "external2_node_tree",
    "custom.frag.glsl"};
