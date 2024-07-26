#version 330
layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec4 plane;

out vec2 TexCoord;
out vec4 ClipSpace;

const float tiling = 6.0;

void main() {
    // Transform the vertex position from local space to clip space.
    gl_Position = projection * view * model * vec4(vPosition, 1.0);
    ClipSpace = gl_Position;
    // Pass along the vertex texture coordinate.
    TexCoord = vec2(vTexCoord.x/2.0 + 0.5, vTexCoord.y/2.0 + 0.5) * tiling;

    gl_ClipDistance[0] = dot(model * vec4(vPosition, 1.0), plane);
}