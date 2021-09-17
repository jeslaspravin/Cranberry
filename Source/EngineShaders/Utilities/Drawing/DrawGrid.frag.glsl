#version 450
#extension GL_GOOGLE_include_directive:enable

#include "../../Common/CommonDefines.inl.glsl"
#include "../../Common/ViewDescriptors.inl.glsl"

layout(location = 0) in vec2 inGrid;
layout(location = 0) out vec4 colorAttachment0;

layout(push_constant) uniform Constants
{
    float gridExtendSize;
    float gridCellSize;
    // Minimum pixels a cell can cover before falling to lower LOD
    float cellMinPixelCoverage;
    vec4 thinColor;
    vec4 thickColor;
} constants;

void mainFS()
{
    // colorAttachment0 = vec4((inGrid / (2 * constants.gridExtendSize)) + 0.5, 0, 1);

    // Change in grid cells size per pixel
    vec2 dGrid = vec2(
        length(vec2(dFdx(inGrid.x), dFdy(inGrid.x)))
        , length(vec2(dFdx(inGrid.y), dFdy(inGrid.y))));

    float lod = max(0.0, LOGBASE(10, (length(dGrid) * constants.cellMinPixelCoverage) / constants.gridCellSize) + 1.0);
    float lodFade = fract(lod);

    float lod0Size = constants.gridCellSize * pow(10, floor(lod));
    float lod1Size = lod0Size * 10;
    float lod2Size = lod1Size * 10;

    // Make it 4x wider
    dGrid *= 4;
    // Now that we have lod size we find that alpha for each lod
    float lod0Alpha = MAX_V2(vec2(1.0) - abs(2 * SATURATE_V2(mod(inGrid, lod0Size) / dGrid) - vec2(1.0)));
    float lod1Alpha = MAX_V2(vec2(1.0) - abs(2 * SATURATE_V2(mod(inGrid, lod1Size) / dGrid) - vec2(1.0)));
    float lod2Alpha = MAX_V2(vec2(1.0) - abs(2 * SATURATE_V2(mod(inGrid, lod2Size) / dGrid) - vec2(1.0)));

    // In lod 2 regions line need to be very thick and lod 0 it needs to be thin
    vec4 outColor = (lod2Alpha > 0.0) 
                        ? constants.thickColor : (lod1Alpha > 0.0)
                            ? mix(constants.thickColor, constants.thinColor, lodFade) : constants.thinColor;
    // Grid global opacity fall off
    float opacityFalloff = 1.0 - SATURATE_F(length(inGrid - viewPos().xy) / constants.gridExtendSize);

    outColor.a *= (lod2Alpha > 0.0) ? lod2Alpha : (lod1Alpha > 0.0) 
                                        ? lod1Alpha : lod0Alpha * (1.0 - lodFade);
    outColor.a *= opacityFalloff;
    colorAttachment0 = outColor;
}