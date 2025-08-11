#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 color;
    vec2 uv;
    vec3 worldPos;
    vec3 worldNrm;
} fs;

uniform sampler2D textureSampler;

// Lights already set from C++:
uniform vec3 sunDir;
uniform vec3 sunColor;

const int MAX_LAMPS = 32;
uniform int  lampCount;
uniform vec3 lampPos[MAX_LAMPS];
uniform vec3 lampColor;
uniform vec3 camPos;

// Headlight spotlights (you set uHL[0], uHL[1] in C++)
struct Headlight {
    vec3  position;
    vec3  direction;   // points *forward* from the lamp
    float innerCut;    // cos(inner angle)   in [0..1], innerCut > outerCut
    float outerCut;    // cos(outer angle)
    float constant;
    float linear;
    float quadratic;
    vec3  color;
    float range;       // max reach in scene units
};
uniform Headlight uHL[2];
uniform bool uUseHeadlights;

// Shadow controls
uniform int   uUseShadow;      // 0 = normal, 1 = shadow pass
uniform vec4  uShadowColor;    // RGBA

float saturate(float x){ return clamp(x, 0.0, 1.0); }

vec3 lambertDir(vec3 N, vec3 L, vec3 C) {
    return C * max(dot(N, normalize(-L)), 0.0);
}

vec3 lambertPoint(vec3 N, vec3 P, vec3 lightPos, vec3 C) {
    vec3 L = lightPos - P;                 // point -> light
    float d = length(L);
    L /= max(d, 1e-4);
    float att = 1.0 / (1.0 + 0.08*d + 0.02*d*d); // matches your Kl/Kq
    return C * max(dot(N, L), 0.0) * att;
}

vec3 blinnSpec(vec3 N, vec3 V, vec3 L, float shininess, float strength) {
    vec3 H = normalize(V + L);
    float s = pow(max(dot(N, H), 0.0), shininess);
    return vec3(strength * s);
}

vec3 spotlight(Headlight h, vec3 N, vec3 P, vec3 V) {
    // Ld: point -> light   (for diffuse/spec)
    vec3  Ld = normalize(h.position - P);
    float d  = length(h.position - P);

    // For the cone, compare light -> point with the headlight's forward direction
    vec3  toFrag = normalize(P - h.position);            // light -> point
    float cosA   = dot(toFrag, normalize(h.direction));  // alignment with beam
    float sm     = smoothstep(h.outerCut, h.innerCut, cosA);  // soft edge

    // Distance attenuation (your constants) + explicit range falloff
    float att = 1.0 / (h.constant + h.linear*d + h.quadratic*d*d);

    // Clamp reach so it dies smoothly between 0.7*range and range
    float rangeFalloff = 1.0 - smoothstep(0.7*h.range, h.range, d); // FIX: h.range
    att *= rangeFalloff;

    vec3 diff = h.color * max(dot(N, Ld), 0.0);
    vec3 spec = blinnSpec(N, V, Ld, 64.0, 0.6) * h.color;

    return (diff + spec) * att * sm;
}

void main()
{
    // Shadow pass: output a tinted translucent black and bail
    if (uUseShadow == 1) {
        FragColor = uShadowColor;
        return;
    }

    vec3  base   = texture(textureSampler, fs.uv).rgb * fs.color;
    vec3  N      = normalize(fs.worldNrm);
    vec3  V      = normalize(camPos - fs.worldPos);

    // Directional sun
    vec3 sunL   = normalize(-sunDir);
    vec3 sunDiff= sunColor * max(dot(N, sunL), 0.0);
    vec3 sunSpec= blinnSpec(N, V, sunL, 64.0, 0.25) * sunColor;

    // Lamps
    vec3 pointSum = vec3(0.0);
    for (int i = 0; i < lampCount && i < MAX_LAMPS; ++i) {
        vec3 L  = normalize(lampPos[i] - fs.worldPos);
        pointSum += lambertPoint(N, fs.worldPos, lampPos[i], lampColor);
        pointSum += blinnSpec(N, V, L, 48.0, 0.15) * lampColor;
    }

    // Headlights (optional)
    vec3 headSum = vec3(0.0);
    if (uUseHeadlights) {
        headSum += spotlight(uHL[0], N, fs.worldPos, V);
        headSum += spotlight(uHL[1], N, fs.worldPos, V);
    }

    vec3 lit = base * (sunDiff + pointSum) + sunSpec + headSum;

    // Very light ambient so unlit texels aren't pitch black
    vec3 ambient = base * 0.12;

    FragColor = vec4(ambient + lit, 1.0);
}
