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

void main() {
    vec2 ndc = (ClipSpace.xy/ClipSpace.w)/2.0 + 0.5;
    vec2 RefractTextCoord = vec2(ndc.x, ndc.y);
    vec2 ReflectTextCoord = vec2(ndc.x, -ndc.y);

    vec4 reflectColor = texture(reflectionTexture, ReflectTextCoord);
    vec4 refractionColor = texture(refractionTexture, RefractTextCoord);

    FragColor = mix(reflectColor, refractionColor, 0.5);
}