#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inVal1;
layout(location = 4) in flat int inVal2;

struct PushConstInnerStruct
{
    float value1;
    vec4 value2;
};

layout(push_constant) uniform PushConstants
{
    layout(offset = 8) float gammaOffset;
    PushConstInnerStruct innerValues;

} pushConstants;

layout(constant_id = 0) const int storePixelCount = 1;
layout(constant_id = 1) const float testVectorType = 20.5f;

struct TempTestData
{
    int indices[4];
    float alpha;
    vec3 alphaOffset;
};

layout(set = 0,binding = 0) uniform ViewData
{
    layout(offset = 144) TempTestData testingData[4];
} viewData;

layout(set = 1,binding = 1) uniform sampler testSampler;
layout(set = 1,binding = 2) uniform texture2D nonSampledImage;

layout(set = 2,binding = 0) uniform sampler2D testTexture;
layout(set = 2,binding = 1, r32f) uniform image2D storeTexture;
layout(set = 2,binding = 3, rgba32f) uniform imageBuffer storePixel[storePixelCount][2];
layout(set = 2,binding = 4) buffer StorageBuffer
{
    vec4 color;
    vec4 position;
} midZonePixels;

layout(input_attachment_index = 0,set = 3,binding = 0) uniform subpassInput attachment0;
layout(input_attachment_index = 1,set = 3,binding = 1) uniform subpassInput attachment1;

layout(location = 0) out vec4 albedoColorAttachment0;
layout(location = 1) out vec4 normalColorAttachment1;
layout(location = 2) out float depthColorAttachment2;

void mainFS()
{
    albedoColorAttachment0 = (inColor * (1 - inVal1) + subpassLoad(attachment0) * inVal1 + texture(testTexture,gl_FragCoord.xy)) + vec4(pushConstants.gammaOffset);
    normalColorAttachment1 = vec4(inNormal,0) + subpassLoad(attachment1);
    depthColorAttachment2 = inPosition.w + inVal2;
    imageStore(storeTexture,ivec2(gl_FragCoord.xy),vec4(inPosition.w));
    imageStore(storePixel[int(gl_FragCoord.x * 1280 + gl_FragCoord.y) % storePixelCount][int(gl_FragCoord.x) % 2], int(gl_FragCoord.x * 1280 + gl_FragCoord.y), inColor);
    midZonePixels.color = albedoColorAttachment0;
    midZonePixels.position = inPosition;
}