#ifdef VERTEX_SHADER
attribute vec4 mc_midTexCoord;

varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec2 vLightUV;
varying vec4 vColor;
varying vec3 vViewDir;

void main() {
    vec4 mv = gbufferModelView * vec4(position, 1.0);
    gl_Position = gbufferProjection * mv;
    vWorldPos = (gbufferModelViewInverse * mv).xyz;
    vViewDir  = normalize(-mv.xyz);
    vNormal   = normalize((gbufferModelViewInverse * vec4(normal, 0.0)).xyz);
    vTexCoord = uv0;
    vLightUV  = (mc_midTexCoord.zw + uv0 * (mc_midTexCoord.xy - mc_midTexCoord.zw)) * (1.0 / 256.0);
    vColor    = color;
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:1 */

varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec2 vLightUV;
varying vec4 vColor;
varying vec3 vViewDir;

uniform sampler2D gtexture;
uniform sampler2D lightmap;
uniform sampler2D colortex0;

void main() {
    vec4 base = texture2D(gtexture, vTexCoord) * vColor;
    if (base.a < 0.01) discard;

    vec4 light     = texture2D(lightmap, vLightUV);
    float skylight = light.a;
    float blocklit = light.r;

    vec3 sunDir = normalize(sunPosition);
    float nDotL = max(0.0, dot(vNormal, sunDir));

    vec3 sunCol   = vec3(1.00, 0.93, 0.78) * SUN_INTENSITY;
    vec3 ambSky   = mix(vec3(0.30, 0.36, 0.46), vec3(0.55, 0.60, 0.70), skylight);
    vec3 ambFloor = vec3(0.20, 0.18, 0.16) * AMBIENT_FLOOR;
    vec3 blockC   = vec3(0.95, 0.78, 0.55) * blocklit * 1.4;

    vec3 direct  = sunCol * nDotL * 0.8;
    vec3 ambient = ambSky * (0.45 + 0.45 * skylight) + ambFloor;

    vec3 lit = base.rgb * (ambient + direct) + blockC * 0.25 * base.rgb;

    vec2 uv = gl_FragCoord.xy / vec2(viewWidth, viewHeight);
    vec3 finalColor;

#if M4WARM_WATER == 0
    finalColor = mix(lit, vec3(0.30, 0.50, 0.62), 0.55);
    gl_FragData[0] = vec4(finalColor, 0.72);

#elif M4WARM_WATER == 1
    vec2 refrOff = vViewDir.xz * 0.04;
    vec3 refrCol = texture2D(colortex0, uv + refrOff).rgb;
    finalColor   = mix(refrCol, lit * vec3(0.35, 0.55, 0.68), 0.55);
    gl_FragData[0] = vec4(finalColor, 0.78);

#else
    vec2 refrOff = vViewDir.xz * 0.035;
    vec2 reflOff = reflect(vViewDir, vec3(0.0, 1.0, 0.0)).xz * 0.06;
    vec3 refrCol = texture2D(colortex0, uv + refrOff).rgb;
    vec3 reflCol = texture2D(colortex0, uv + reflOff).rgb;
    float fres   = pow(clamp(1.0 - vViewDir.y, 0.0, 1.0), 3.0);
    fres         = mix(0.04, 1.0, fres);
    vec3 surface = mix(refrCol, reflCol, fres);
    vec3 tint    = vec3(0.30, 0.55, 0.66);
    finalColor   = mix(surface, tint, 0.45);
    finalColor   = mix(finalColor, lit, 0.15);
    gl_FragData[0] = vec4(finalColor, 0.80);
#endif
}
#endif
