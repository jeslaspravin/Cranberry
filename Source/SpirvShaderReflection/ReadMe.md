This reflection console application is implemented based on SPIRV-Cross reflection library(https://github.com/KhronosGroup/SPIRV-Cross).

Not all the data type and features of SPIRV-Cross are supported already.

Data type supported 
	<ul>
		<li>bool</li>
		<li>uint32</li>
		<li>int32</li>
		<li>float</li>
		<li>double</li>
	</ul>
as far as vectors and matrices are concerned, vector size from 1 to 4 is supported and matrices from 1 to 4 is supported.
In Images or textures Comparision sampling is not supported right now.

Spirv shader files for each stage should be seperate files

Reflected data are stored in the form of binaries to file path provided as last two commandline arguments.
* argument at n-2 should be file where the reflected data should be written to.
* argument at n-1 should be file where the combined shader code has to be written to.

All the arguments specified from 0 to n-3 will be used as spirv shader file path that needs to be reflected for a single pipeline.

Example
app.exe shader1.glsl shader2.glsl fileRef.ref combinedShader.shader

This application shares code base in form of shared library EngineShaderData(https://github.com/jeslaspravin/GameEngine/tree/master/Source/EngineShaderData) with Engine itself.

In case of supporting for new format or stages
	<ul>
		<li>Add neccessary changes to ShaderDataTypes.h</li>
		<li>Include required archive << overrides changes</li>
		<li>Add changes to utility handling in ShaderReflectionProcessor.cpp</li>
		<li>Make neccessary modification to pipeline and shader data generation in the engine codes</li>
		<li>Introduce neccessary cross verification changes</li>
		<li>Double check every thing manually once more</li>
	</ul>