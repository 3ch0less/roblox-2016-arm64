#ifdef VERTEX_SHADER
void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:3 */

uniform sampler2D gaux1;
uniform sampler2D depthtex0;
uniform sampler2D colortex0;

void main() {
#if RT_ENABLED == 0
    gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
    return;
#else

#if RT_SSR_HALF_RES == 1
    vec2 uv = (gl_FragCoord.xy * 2.0) / vec2(viewWidth, viewHeight);
#else
    vec2 uv = gl_FragCoord.xy / vec2(viewWidth, viewHeight);
#endif

    float depth = texture2D(depthtex0, uv).r;
    if (depth >= 0.9999) {
        gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 normData = texture2D(gaux1, uv).rgb;
    vec3 normal   = decodeNormal(normData);
    float roughness = normData.a;

    vec3 viewPos = reconstructViewPos(uv, depth);

    float ao = 1.0;
    vec3 reflColor = vec3(0.0);

#if RT_AO_ON == 1
    ao = rtao(uv, viewPos, normal);
#endif

#if RT_SSR_ON == 1
    reflColor = ssr(uv, viewPos, normal, roughness, depth);
#endif

    gl_FragData[0] = vec4(reflColor, ao);
#endif
}
#endif
