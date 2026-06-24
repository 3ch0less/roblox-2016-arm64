#ifdef VERTEX_SHADER
void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:0 */

uniform sampler2D colortex0;
uniform sampler2D colortex3;
uniform sampler2D gaux1;
uniform sampler2D depthtex0;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(viewWidth, viewHeight);

    vec3 scene = texture2D(colortex0, uv).rgb;

#if RT_ENABLED == 0
    gl_FragData[0] = vec4(scene, 1.0);
    return;
#else

#if RT_SSR_HALF_RES == 1
    vec2 halfUV = uv * 0.5;
    vec2 texelSize = vec2(2.0 / viewWidth, 2.0 / viewHeight);
#else
    vec2 halfUV = uv;
    vec2 texelSize = vec2(1.0 / viewWidth, 1.0 / viewHeight);
#endif

    vec3 rtData = bilateralFilter(colortex3, halfUV, texelSize);

    float ao = rtData.a;
    vec3 reflColor = rtData.rgb;

    float roughness = texture2D(gaux1, uv).a;
    float reflStrength = 1.0 - roughness;
    reflStrength = reflStrength * reflStrength;

    float aoFactor = mix(RT_AO_INTENSITY, 1.0, ao);
    scene *= aoFactor;

    scene = mix(scene, reflColor, reflStrength * RT_SSR_INTENSITY);

    gl_FragData[0] = vec4(scene, 1.0);
#endif
}
#endif
