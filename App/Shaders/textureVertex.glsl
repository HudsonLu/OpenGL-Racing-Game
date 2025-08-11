#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aNormal;
// Instance transformation matrix. When not using instancing the first
// element is set to identity on the CPU side and only element 0 is used.
uniform mat4 instanceMatrices[100];

uniform mat4 camMatrix;
uniform mat4 model;

// Shadow controls
uniform int   uUseShadow;        // 0 = normal draw, 1 = shadow pass
uniform float uShadowPlaneY;     // the ground plane Y you’re projecting onto
uniform float uShadowMinBias;    // tiny lift above the plane to avoid z-fighting

out VS_OUT {
    vec3 color;
    vec2 uv;
    vec3 worldPos;
    vec3 worldNrm;
} vs;

void main()
{
    // World space
    mat4 inst = instanceMatrices[gl_InstanceID];
    mat4 fullModel = model * inst;
    vec4 wp = fullModel * vec4(aPos, 1.0);
    vec3 wn = mat3(transpose(inverse(fullModel))) * aNormal;

    // During the shadow pass, clamp Y to never go below the plane
    // (the matrix you use on CPU already projects to the plane;
    // this is a last safety net that *forces* Y >= plane+bias)
    if (uUseShadow == 1) {
        wp.y = uShadowPlaneY + uShadowMinBias;
    }

    vs.color    = aColor;
    vs.uv       = aUV;
    vs.worldPos = wp.xyz;
    vs.worldNrm = normalize(wn);

    gl_Position = camMatrix * wp;
}
