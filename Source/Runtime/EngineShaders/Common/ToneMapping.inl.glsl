#ifndef TONEMAPPING_INCLUDE
#define TONEMAPPING_INCLUDE

#define GAMMA_CORRECT(color, gamma) pow((color), vec3(1.0/(gamma))) 

// reinhard tone mapping
// https://learnopengl.com/Advanced-Lighting/HDR
vec3 reinhardToneMap(vec3 inColor)
{
    return inColor / (inColor + vec3(1.0));
}


// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 inColor)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((inColor * (A * inColor + C * B) + D * E)/(inColor * (A * inColor + B) + D * F)) - E/F;
}
vec3 Uncharted2Tonemap(vec3 inColor, float inExposure)
{
	vec3 color = Uncharted2Tonemap(inColor * inExposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    return color;
}

#endif // TONEMAPPING_INCLUDE