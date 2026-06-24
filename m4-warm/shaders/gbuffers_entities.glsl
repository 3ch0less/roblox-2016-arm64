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
uniform sampler2D shadowtex0;
uniform mat4 shadowModelView;
uniform mat4 shadowProjection;

void main() {
    vec4 base = texture2D(gtexture, vTexCoord) * vColor;
    if (base.a < 0.5) discard;

    vec4 light     = texture2D(lightmap, vLightUV);
    float skylight = light.a;
    float blocklit = light.r;

    vec3 sunDir = normalize(sunPosition);
    float nDotL = max(0.0, dot(vNormal, sunDir));

    float shadow = 1.0;
#if SHADOW_ENABLED == 1
    vec4 sp = shadowProjection * (shadowModelView * vec4(vWorldPos, 1.0));
    vec3 sc = sp.xyz / sp.w * 0.5 + 0.5;
    if (clamp(sc.x, 0.0, 1.0) == sc.x && clamp(sc.y, 0.0, 1.0) == sc.y && sc.z > 0.0) {
        shadow = pcf2x2(shadowtex0, sc, 0.0009);
    }
    float dayFactor = smoothstep(-0.05, 0.10, sunDir.y);
    shadow = mix(1.0, mix(0.60, 1.0, shadow), dayFactor);
#endif

    vec3 sunCol   = vec3(1.00, 0.93, 0.78) * SUN_INTENSITY;
    vec3 ambSky   = mix(vec3(0.30, 0.36, 0.46), vec3(0.55, 0.60, 0.70), skylight);
    vec3 ambFloor = vec3(0.20, 0.18, 0.16) * AMBIENT_FLOOR;
    vec3 blockC   = vec3(0.95, 0.78, 0.55) * blocklit * 1.4;

    vec3 direct  = sunCol * nDotL * shadow;
    vec3 ambient = ambSky * (0.50 + 0.45 * skylight) + ambFloor;
    vec3 moonCol = vec3(0.78, 0.82, 0.95) * MOON_INTENSITY;
    vec3 moon    = moonCol * max(0.0, dot(vNormal, normalize(moonPosition))) * (1.0 - smoothstep(-0.05, 0.10, sunDir.y));

    vec3 lit = base.rgb * (ambient + direct + moon) + blockC * 0.30 * base.rgb;

    float depth = gl_FragCoord.z;
    float fogF  = clamp((depth - 0.94) / 0.06, 0.0, 1.0);
    vec3 fogCol = mix(FOG_TINT_NIGHT, FOG_TINT_DAY, clamp(sunDir.y * 0.5 + 0.5, 0.0, 1.0));
    lit = mix(lit, fogCol, fogF);

    vec3 nEnc = encodeNormal(vNormal);

    gl_FragData[0] = vec4(lit, base.a);
    gl_FragData[1] = vec4(nEnc, RT_ROUGHNESS_ENTITIES);
}
#endif
