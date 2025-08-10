#version 330 core

out vec4 FragColor;

in vec3 vertexColor;
in vec2 vertexUV;
in vec3 vertexNormal;
in vec3 crntPos;

uniform sampler2D textureSampler;

// Existing point/ambient/spec uniforms you already use
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

// ==== NEW: Headlight spotlights ====
struct SpotLight {
    vec3 position;    // world
    vec3 direction;   // world (normalized)
    float innerCut;   // cos(innerAngle)
    float outerCut;   // cos(outerAngle)
    float constant;
    float linear;
    float quadratic;
    vec3 color;
};

uniform bool  uUseHeadlights;
uniform SpotLight uHL[2];
// ================================

vec3 spotContrib(SpotLight L, vec3 N, vec3 P, vec3 V, vec3 albedo)
{
    vec3 Ldir = normalize(L.position - P);
    float theta   = dot(Ldir, normalize(-L.direction)); // alignment with beam axis
    float epsilon = max(L.innerCut - L.outerCut, 1e-4);
    float spot    = clamp((theta - L.outerCut) / epsilon, 0.0, 1.0);

    float dist  = length(L.position - P);
    float atten = 1.0 / (L.constant + L.linear * dist + L.quadratic * dist * dist);

    float NdotL = max(dot(N, Ldir), 0.0);
    vec3 diffuse = albedo * NdotL;

    // simple Blinn-Phong-ish spec
    vec3 H = normalize(Ldir + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    return (diffuse + 0.2*spec) * L.color * atten * spot;
}

void main()
{
    // Base material
    vec3 albedo = texture(textureSampler, vertexUV).rgb;

    // Your existing lighting (ambient + point light + spec)
    float ambientStrength = 0.20;
    vec3  N = normalize(vertexNormal);
    vec3  LdirPoint = normalize(lightPos - crntPos);
    float diffPoint = max(dot(N, LdirPoint), 0.0);

    float specularLight = 0.50;
    vec3  V = normalize(camPos - crntPos);
    vec3  R = reflect(-LdirPoint, N);
    float specAmount = pow(max(dot(V, R), 0.0), 8.0);
    float specPoint   = specAmount * specularLight;

    vec3 color = albedo * lightColor.rgb * (ambientStrength + diffPoint) + lightColor.rgb * specPoint;

    // ==== NEW: add two headlight spot contributions ====
    if (uUseHeadlights) {
        color += spotContrib(uHL[0], N, crntPos, V, albedo);
        color += spotContrib(uHL[1], N, crntPos, V, albedo);
    }
    // ===================================================

    FragColor = vec4(color, 1.0);
}
