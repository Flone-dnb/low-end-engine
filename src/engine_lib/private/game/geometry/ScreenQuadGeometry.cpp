#include "game/geometry/ScreenQuadGeometry.h"

ScreenQuadGeometry::ScreenQuadGeometry(
    const std::array<VertexLayout, iVertexCount>& vVertices, std::unique_ptr<VertexArrayObject> pQuadVao)
    : vVertices(vVertices), pQuadVao(std::move(pQuadVao)) {}
