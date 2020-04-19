#include "TriangleShader.h"

DEFINE_GRAPHICS_RESOURCE(ExperimentalTriangleShader)

ExperimentalTriangleShader::ExperimentalTriangleShader()
    : BaseType("ExperimentalTriangle")
{
    String filePath;
    filePath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(filePath), "Shaders", getResourceName());

    SharedPtr<ShaderCodeResource> vertexCode(shaderCodeFactory(this, filePath + "." 
        + EShaderStage::getShaderStageInfo(EShaderStage::Vertex)->shortName + "." + SHADER_EXTENSION));
    SharedPtr<ShaderCodeResource> fragCode(shaderCodeFactory(this, filePath + "." 
        + EShaderStage::getShaderStageInfo(EShaderStage::Fragment)->shortName + "." + SHADER_EXTENSION));
    shaders.clear();
    shaders[EShaderStage::Vertex] = vertexCode;
    shaders[EShaderStage::Fragment] = fragCode;
}
