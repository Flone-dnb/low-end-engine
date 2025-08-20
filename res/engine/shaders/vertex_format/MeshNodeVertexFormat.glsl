// Make sure shader type macro is defined.
#if !defined(VERTEX_SHADER) && !defined(FRAGMENT_SHADER) // shader manager defined this macro
    // Expected a layout macro to be defined.
    SHADER_TYPE_MACRO_EXPECTED;
#endif

#ifdef VERTEX_SHADER
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 uv;
#else
    in vec3 fragmentPosition;
    in vec3 fragmentNormal;
    in vec2 fragmentUv;
#endif
