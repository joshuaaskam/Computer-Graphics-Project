#version 330
layout (location=0) out vec4 FragColor;

in vec2 TexCoord;
in vec4 ClipSpace;
in vec3 toCameraVector;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D dudvMap;

uniform float moveFactor;

const float waveStrength = 0.005;

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

    // This value will be used to determine how clear vs reflective the water is.
    // At angles where the camera is looking straight down at the water, there is less reflection (Clear water).
    // Low angles mean the water has more reflection.
    vec3 viewVector = normalize(toCameraVector);
    float refractiveFactor = dot(viewVector, vec3(0.0, 1.0, 0.0));
    refractiveFactor = pow(refractiveFactor, 0.7);

    FragColor = mix(reflectColor, refractionColor, refractiveFactor);
    FragColor = mix(FragColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2);
}