#version 330
// A fragment shader for rendering fragments in the Phong reflection model.
layout (location=0) out vec4 FragColor;

// Inputs: the texture coordinates, world-space normal, and world-space position
// of this fragment, interpolated between its vertices.
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragWorldPos;
in vec4 ClipSpace;

// Uniforms: MUST BE PROVIDED BY THE APPLICATION.

// The mesh's base (diffuse) texture.
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D dudvMap;

uniform float moveFactor;

const float waveStrength = 0.01; //0.005

void main() {
    vec2 ndc = (ClipSpace.xy/ClipSpace.w)/2.0 + 0.5;
    vec2 RefractTextCoord = vec2(ndc.x, ndc.y);
    vec2 ReflectTextCoord = vec2(ndc.x, -ndc.y);

    // Add distortion to simulate ripples in the water
    vec2 distortion1 = (texture(dudvMap, vec2(TexCoord.x + moveFactor, TexCoord.y)).rg * 2.0 - 1.0) * waveStrength;
    vec2 distortion2 = (texture(dudvMap, vec2(-TexCoord.x + moveFactor, TexCoord.y + moveFactor)).rg * 2.0 - 1.0) * waveStrength;
    vec2 totalDistortion = distortion1 + distortion2;

    RefractTextCoord += totalDistortion;
    RefractTextCoord = clamp(RefractTextCoord, 0.001, 0.999);

    ReflectTextCoord += totalDistortion;
    ReflectTextCoord.x = clamp(ReflectTextCoord.x, 0.001, 0.999);
    ReflectTextCoord.y = clamp(ReflectTextCoord.y, -0.999, -0.001);

    vec4 reflectColor = texture(reflectionTexture, ReflectTextCoord);
    vec4 refractionColor = texture(refractionTexture, RefractTextCoord);

    FragColor = mix(reflectColor, refractionColor, 0.5);
    FragColor = mix(FragColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2);
}