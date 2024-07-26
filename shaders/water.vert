#version 330
// A vertex shader for rendering water with projected texture coordinates,
layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec4 plane;
uniform vec3 viewPos;
uniform vec3 lightPos;

out vec2 TexCoord;
out vec4 ClipSpace;
out vec3 toCameraVector;
out vec3 fromLightVector;

const float tiling = 4.0;

void main() {
    // Transform the vertex position from local space to clip space.
    vec4 worldPos = model * vec4(vPosition, 1.0);
    gl_Position = projection * view * worldPos;
    ClipSpace = gl_Position;

    // Pass along the projected vertex texture coordinate.
    TexCoord = vec2(vTexCoord.x/2.0 + 0.5, vTexCoord.y/2.0 + 0.5) * tiling;

    toCameraVector = viewPos - worldPos.xyz;

    fromLightVector = worldPos.xyz - lightPos;

    gl_ClipDistance[0] = dot(model * vec4(vPosition, 1.0), plane);
}