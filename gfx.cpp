#include "gfx.hpp"

#include <CafeGLSLCompiler.h>
#include <camera/camera.h>
#include <cstring>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/texture.h>
#include <gx2/utils.h>
#include <whb/gfx.h>
#include <whb/log.h>

namespace gfx
{

constexpr float s_texCoords[8]{0.0f, 1.0, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};

constexpr float xScale = 640.0f / 854.0f;
constexpr float s_posCoords[8]{-xScale, -1.0f, +xScale, -1.0f, +xScale, +1.0f, -xScale, +1.0f};

constexpr const char* vertShaderSrc = R"(
#version 450

layout (location = 0) in vec2 inPosCoord;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec2 texCoord;

void main()
{
    gl_Position = vec4(inPosCoord.x, inPosCoord.y, 0.0, 1.0);
    texCoord = inTexCoord;
}
)";

constexpr const char* fragShaderSrc = R"(
#version 450

layout (location = 0) in vec2 texCoord;

layout (binding = 0) uniform sampler2D yTex;
layout (binding = 1) uniform sampler2D uvTex;

out vec4 FragColor;

void main()
{
    float y = texture(yTex, texCoord).r;
    vec2 uvVec = texture(uvTex, texCoord).rg;

    float yA = y - 0.0625;
    float uA = uvVec.r - 0.5;
    float vA = uvVec.g - 0.5;

    float r = yA + 1.13983 * vA;
    float g = yA - 0.39465 * uA - 0.58060 * vA;
    float b = yA + 2.03211 * uA;
    vec3 posCol = vec3(texCoord.rg, 0.01 * g);
    FragColor = vec4(r, g, b, 1.0);
})";

WHBGfxShaderGroup s_shaderGroup{};
GX2Texture s_yTex{};
GX2Texture s_uvTex{};
GX2Sampler s_sampler{};

void InitTex(GX2Texture& tex, uint32_t width, uint32_t height, GX2SurfaceFormat surfaceFormat, uint32_t compMap)
{
    tex.viewNumMips = 1;
    tex.viewFirstMip = 0;
    tex.viewNumSlices = 1;
    tex.viewFirstSlice = 0;
    tex.surface.mipLevels = 1;
    tex.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
    tex.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    tex.surface.use = GX2_SURFACE_USE_TEXTURE;
    tex.surface.aa = GX2_AA_MODE1X;
    tex.surface.depth = 1;
    tex.surface.width = width;
    tex.surface.height = height;
    tex.surface.format = surfaceFormat;
    tex.compMap = compMap;

    GX2CalcSurfaceSizeAndAlignment(&tex.surface);
    GX2InitTextureRegs(&tex);

    WHBLogPrintf("Image: Size: %d, Alignment: %d, Pitch: %d", tex.surface.imageSize, tex.surface.alignment,
                 tex.surface.pitch);
}

bool Init()
{
    InitTex(s_yTex, CAMERA_WIDTH, CAMERA_HEIGHT, GX2_SURFACE_FORMAT_UNORM_R8,
            GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_0, GX2_SQ_SEL_0, GX2_SQ_SEL_1));
    InitTex(s_uvTex, CAMERA_WIDTH / 2, CAMERA_HEIGHT / 2, GX2_SURFACE_FORMAT_UNORM_R8_G8,
            GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_0, GX2_SQ_SEL_1));

    GX2InitSampler(&s_sampler, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);
    GX2InitSamplerBorderType(&s_sampler, GX2_TEX_BORDER_TYPE_BLACK);

    constexpr static auto infoLogSize = 512;
    char infoLog[infoLogSize];

    s_shaderGroup.vertexShader = GLSL_CompileVertexShader(vertShaderSrc, infoLog, infoLogSize, GLSL_COMPILER_FLAG_NONE);
    if (!s_shaderGroup.vertexShader)
    {
        WHBLogPrintf("Failed to compile vertex shader: %s", infoLog);
        return false;
    }
    WHBLogPrintf("Vertex Shader: %d Attrib, %d Samplers, %d Uniforms", s_shaderGroup.vertexShader->attribVarCount,
                 s_shaderGroup.vertexShader->samplerVarCount, s_shaderGroup.vertexShader->uniformVarCount);

    s_shaderGroup.pixelShader = GLSL_CompilePixelShader(fragShaderSrc, infoLog, infoLogSize, GLSL_COMPILER_FLAG_NONE);
    if (!s_shaderGroup.pixelShader)
    {
        WHBLogPrintf("Failed to compile pixel shader: %s", infoLog);
        GLSL_FreeVertexShader(s_shaderGroup.vertexShader);
        return false;
    }
    WHBLogPrintf("Pixel Shader: %d Samplers, %d Uniforms", s_shaderGroup.pixelShader->samplerVarCount,
                 s_shaderGroup.pixelShader->uniformVarCount);

    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, s_shaderGroup.vertexShader->program,
                  s_shaderGroup.vertexShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, s_shaderGroup.pixelShader->program, s_shaderGroup.pixelShader->size);

    // Shader attributes

    WHBGfxInitShaderAttribute(&s_shaderGroup, "inPosCoord", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    WHBGfxInitShaderAttribute(&s_shaderGroup, "inTexCoord", 1, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);

    WHBGfxInitFetchShader(&s_shaderGroup);

    return true;
}

void SetImage(void* img)
{
    if (img == s_yTex.surface.image)
        return;
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, img, CAMERA_YUV_BUFFER_SIZE);
    s_yTex.surface.image = img;
    s_uvTex.surface.image = static_cast<uint8_t*>(img) + CAMERA_Y_BUFFER_SIZE;
}

void DrawInternal()
{
    WHBGfxClearColor(0.6, 0.3, 0.3, 1.0);

    GX2SetFetchShader(&s_shaderGroup.fetchShader);
    GX2SetVertexShader(s_shaderGroup.vertexShader);
    GX2SetPixelShader(s_shaderGroup.pixelShader);

    GX2SetPixelTexture(&s_yTex, s_shaderGroup.pixelShader->samplerVars[0].location);
    GX2SetPixelTexture(&s_uvTex, s_shaderGroup.pixelShader->samplerVars[1].location);

    GX2SetPixelSampler(&s_sampler, s_shaderGroup.pixelShader->samplerVars[0].location);
    GX2SetPixelSampler(&s_sampler, s_shaderGroup.pixelShader->samplerVars[1].location);

    GX2SetAttribBuffer(0, 4 * 2 * sizeof(float), 2 * sizeof(float), s_posCoords);
    GX2SetAttribBuffer(1, 4 * 2 * sizeof(float), 2 * sizeof(float), s_texCoords);

    GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, 4, 0, 1);
}

void Draw()
{
    WHBGfxBeginRender();

    WHBGfxBeginRenderTV();
    DrawInternal();
    WHBGfxFinishRenderTV();

    WHBGfxBeginRenderDRC();
    DrawInternal();
    WHBGfxFinishRenderDRC();

    WHBGfxFinishRender();
}

void Cleanup()
{
    WHBGfxFreePixelShader(s_shaderGroup.pixelShader);
    WHBGfxFreeVertexShader(s_shaderGroup.vertexShader);
}
}; // namespace gfx
