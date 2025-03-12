#pragma once

// Standard.
#include <string>

class ShaderManager;

/**
 * Compiled GLSL shader.
 *
 * @remark Deletes GL shader program in destructor.
 */
class Shader {
    // Only shader manager is allowed to create objects of this type.
    friend class ShaderManager;

public:
    Shader() = delete;

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    ~Shader();

    /**
     * Returns path to .glsl file relative `res` directory.
     *
     * @return Path to .glsl file.
     */
    std::string getPathToShaderRelativeRes() const { return sPathToShaderRelativeRes; }

    /**
     * Returns OpenGL ID of the compiled shader.
     *
     * @return ID.
     */
    unsigned int getShaderId() { return iShaderId; }

private:
    /**
     * Creates a new shader.
     *
     * @param pShaderManager           Manager that created this shader.
     * @param sPathToShaderRelativeRes Path to .glsl file relative `res` directory.
     * @param iShaderId                OpenGL ID of the compiled shader.
     */
    Shader(
        ShaderManager* pShaderManager, const std::string& sPathToShaderRelativeRes, unsigned int iShaderId);

    /** OpenGL ID of the compiled shader. */
    const unsigned int iShaderId = 0;

    /** Path to .glsl file relative `res` directory. */
    const std::string sPathToShaderRelativeRes;

    /** Manager that created this shader. */
    ShaderManager* const pShaderManager = nullptr;
};
