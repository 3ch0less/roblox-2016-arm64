#ifdef VERTEX_SHADER
attribute vec4 mc_midTexCoord;

varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec2 vLightUV;
varying vec4 vColor;

void main() {
    vec4 mv = gbufferModelView * vec4(position, 1.0);
    gl_Position = gbufferProjection * mv;
    vWorldPos  = (gbufferModelViewInverse * mv).xyz;
    vNormal    = normalize((gbufferModelViewInverse * vec4(normal, 0.0)).xyz);
    vTexCoord  = uv0;
    vLightUV   = (mc_midTexCoord.zw + uv0 * (mc_midTexCoord.xy - mc_midTexCoord.zw)) * (1.0 / 256.0);
    vColor     = color;
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:08 */

varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec2 vLightUV;
varying vec4 vColor;

uniform sampler2D gtexture;
uniform sampler2D lightmap;

void main() {
    vec4 base = texture2D(gtexture, vTexCoord) * vColor;
    if (base.a < 0.5) discard;

    vec4 light     = texture2D(lightmap, vLightUV);
    float skylight = light.a;
    float blocklit = light.r;

    vec3 sunDir = normalize(sunPosition);
    float nDotL = max(0.0, dot(vNormal, sunDir));

    vec3 sunCol   = vec3(1.00, 0.93, 0.78) * SUN_INTENSITY;
    vec3 ambSky   = mix(vec3(0.30, 0.36, 0.46), vec3(0.55, 0.60, 0.70), skylight);
    vec3 ambFloor = vec3(0.20, 0.18, 0.16) * AMBIENT_FLOOR;
    vec3 blockC   = vec3(0.95, 0.78, 0.55) * blocklit * 1.4;

    vec3 direct  = sunCol * nDotL;
    vec3 ambient = ambSky * (0.55 + 0.40 * skylight) + ambFloor;
    vec3 moon    = vec3(0.78, 0.82, 0.95) * MOON_INTENSITY
                 * max(0.0, dot(vNormal, normalize(moonPosition)))
                 * (1.0 - smoothstep(-0.05, 0.10, sunDir.y));

    vec3 lit = base.rgb * (ambient + direct + moon) + blockC * 0.30 * base.rgb;

    vec3 nEnc = encodeNormal(vNormal);

    gl_FragData[0] = vec4(lit, 1.0);
    gl_FragData[1] = vec4(nEnc, RT_ROUGHNESS_HAND);
}
#endif
