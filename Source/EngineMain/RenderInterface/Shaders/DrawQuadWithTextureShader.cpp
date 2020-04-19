#include "DrawQuadWithTextureShader.h"

DEFINE_GRAPHICS_RESOURCE(DrawQuadWithTextureShader);

DrawQuadWithTextureShader::DrawQuadWithTextureShader() : BaseType("DrawQuadWithTexture")
{
    String filePath;
    filePath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(filePath), "Shaders", "DrawQuad");

    SharedPtr<ShaderCodeResource> vertexCode(shaderCodeFactory(this, filePath + "." 
        + EShaderStage::getShaderStageInfo(EShaderStage::Vertex)->shortName + "." + SHADER_EXTENSION));
    SharedPtr<ShaderCodeResource> fragCode(shaderCodeFactory(this, filePath + "."
        + EShaderStage::getShaderStageInfo(EShaderStage::Fragment)->shortName + "." + SHADER_EXTENSION));
    shaders.clear();
    shaders[EShaderStage::Vertex] = vertexCode;
    shaders[EShaderStage::Fragment] = fragCode;
}

