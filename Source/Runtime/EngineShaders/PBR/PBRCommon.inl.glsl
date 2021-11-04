#ifndef PBRCOMMON_INCLUDE
#define PBRCOMMON_INCLUDE

#include "../Common/CommonDefines.inl.glsl"

// Low discrepancy sequence generators(Quasi-Monte carlo)
// Van Der Corput
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverseVdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint nSamples)
{
    return vec2(float(i)/float(nSamples), radicalInverseVdC(i));
}

// Distribution, Geometry Masks, BXDFs

// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSampleGGX(vec2 uniformSample, vec3 normal, float roughness)
{
    float a = SQR(roughness);
	
    float phi = 2.0 * M_PI * uniformSample.x;
    float cosTheta = sqrt((1.0 - uniformSample.y) / (1.0 + (SQR(a) - 1.0) * uniformSample.y));
    float sinTheta = sqrt(1.0 - SQR(cosTheta));
	
    // from spherical coordinates to cartesian coordinates
    vec3 localHalfVec;
    localHalfVec.x = cos(phi) * sinTheta;
    localHalfVec.y = sin(phi) * sinTheta;
    localHalfVec.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 bitan   = normalize(cross(normal, (abs(normal.x) < 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 0.0, 1.0))));
    vec3 tangent = cross(bitan, normal);
	
    vec3 sampleVec = tangent * localHalfVec.x + bitan * localHalfVec.y + normal * localHalfVec.z;
    return normalize(sampleVec);
}

float ggxtrNDF(vec3 normal, vec3 halfVector, float roughness)
{
    float alpha = SQR(roughness);
    float alpha2 = SQR(alpha);
    float temp = max(dot(normal, halfVector), 0);
    temp = (SQR(temp) * (alpha2 - 1)) + 1;
    return alpha2 / (M_PI * SQR(temp));
}

// For lights k = ((rough + 1) ^ 2) / 8
// IBL k = (rough ^ 2)/2
float geoMaskSchlickGGX(vec3 normal, vec3 maskedDir, float k)
{
    float nDotD = max(dot(normal, maskedDir), 0.0);
    return nDotD/(nDotD * (1 - k) + k);
}

float geoMaskSmith(vec3 normal, vec3 pt2ViewDir, vec3 pt2LightDir, float k)
{
    return geoMaskSchlickGGX(normal, pt2ViewDir, k) * geoMaskSchlickGGX(normal, pt2LightDir, k);
}

vec3 fresnelSchlick(vec3 halfVec, vec3 viewDir, vec3 f0)
{
    return f0 + (1 - f0) * pow((1 - max(dot(halfVec, viewDir), 0.0)), 5);
}
vec3 fresnelSchlickRoughness(vec3 halfVec, vec3 viewDir, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1 - roughness), f0) - f0) * pow((1 - max(dot(halfVec, viewDir), 0.0)), 5);
}

vec2 integrateBRDF(float nDotV, float roughness, uint sampleCount)
{
    vec3 pt2View = vec3(sqrt(1.0 - SQR(nDotV)), 0.0, nDotV);
    const vec3 normal = vec3(0.0, 0.0, 1.0);

    float brdf = 0.0;
    float bias = 0.0;

    for(uint i = 0u; i < sampleCount; ++i)
    {
        vec2 uniSample = hammersley(i, sampleCount);
        vec3 halfVector = importanceSampleGGX(uniSample, normal, roughness);
        vec3 pt2Light = 2.0 * dot(pt2View, halfVector) * halfVector - pt2View;

        float nDotL = max(pt2Light.z, 0.0);
        float nDotH = max(halfVector.z, 0.0);
        float pt2vDotH = max(dot(pt2View, halfVector), 0.0);

        if(nDotL > 0.0)
        {
            float geomMask = geoMaskSmith(normal, pt2View, pt2Light, SQR(roughness) * 0.5);
            float geomVis = (geomMask * pt2vDotH) / (nDotH * nDotV);
            float fresnelConst = pow(1.0 - pt2vDotH, 5.0);

            brdf += (1.0 - fresnelConst) * geomVis;
            bias += fresnelConst * geomVis;
        }
    }
    return vec2(brdf, bias) / vec2(sampleCount);
}

// Epic games fall off
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float falloff(float falloffDistance, float maxRadius ) 
{
	float falloff = 0;
	float distOverRadius = falloffDistance / maxRadius;
	float distOverRadius4 = SQR(SQR(distOverRadius));
	falloff = clamp(1.0 - distOverRadius4, 0.0, 1.0);
    falloff = SQR(falloff);
// Scaling to meters
	falloff /= SQR(falloffDistance * M_CM2M) + 1.0;
	return falloff;
}

#endif // PBRCOMMON_INCLUDE