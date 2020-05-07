#version 450
// Has Vertex input(vec3,vec2, float ,int ) , Push constant's one value , one input attachment, ubo at s 0 b 0,s 1 b 0 ,texel read buffer at s 2 b 2

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBiTangent;
layout(location = 4) in vec2 inUV;
layout(location = 5) in float inValue1;
layout(location = 6) in int inValue2;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out float outVal1;
layout(location = 4) out int outVal2;

struct PushConstInnerStruct
{
    float value1;
    vec4 value2;
};

layout(push_constant) uniform PushConstants
{
    float rotation;
    layout(offset = 16)PushConstInnerStruct innerValues;// Struct, vec3,4, matrix are 16byte aligned
} pushConstants;

struct TempTestData
{
    int indices[4];
    float alpha;
    vec3 alphaOffset;
};

layout(set = 0,binding = 0) uniform ViewData
{ 
    mat4 viewProjection;
    mat4 invModelView;
    vec2 viewSize;
    TempTestData testingData[4];
} viewData;


layout(set = 1,binding = 0) uniform ObjectData 
{  
    mat4 localToWorld;
    mat4 worldToLocal;
    vec4 bound[2];
} objectData;

layout(set = 1,binding = 1) uniform sampler testSampler;
layout(set = 1,binding = 2) uniform texture2D nonSampledImage;

layout(set = 2,binding = 2) uniform samplerBuffer vertexColors;

// Just some random calculations nothing means anything
void mainVS()
{
    vec4 transformedPos = viewData.viewProjection * objectData.localToWorld * vec4(inPosition,1);
    outVal1 = inValue1 * pushConstants.rotation;
    outNormal = (viewData.viewProjection * objectData.localToWorld * vec4(inNormal,1)).xyz;
    outColor = texelFetch(vertexColors,gl_VertexIndex);
    gl_Position = transformedPos;
    outPosition = (transformedPos/ viewData.viewSize.x)+viewData.viewSize.y;
    outVal2 = int(inValue2 * 4 * objectData.bound[0].x + inValue1 * objectData.bound[1].y);
}