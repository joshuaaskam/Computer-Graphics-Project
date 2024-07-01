#version 330
// A fragment shader for rendering fragments in the Phong reflection model.
layout (location=0) out vec4 FragColor;

// Inputs: the texture coordinates, world-space normal, and world-space position
// of this fragment, interpolated between its vertices.
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragWorldPos;

// Uniforms: MUST BE PROVIDED BY THE APPLICATION.

// The mesh's base (diffuse) texture.
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;

void main() {
    vec4 reflectColor = texture(reflectionTexture, TexCoord);
    vec4 refractionColor = texture(refractionTexture, TexCoord);

    vec4 color = mix(reflectColor, refractionColor, 0.5);
    if(color.r == 0.0 && color.g == 0.0 && color.b == 0) {
        FragColor = vec4(0.65f, 0.8f, 0.92f, 1.0);
    }
    else {
        FragColor = mix(reflectColor, refractionColor, 0.5);
    }
}