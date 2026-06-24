#ifdef VERTEX_SHADER
void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:2 */

uniform sampler2D colortex0;
uniform sampler2D depthtex0;
uniform mat4 gbufferModelView;
uniform mat4 gbufferProjection;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(viewWidth, viewHeight);

#if GODRAY_ENABLED == 0
    gl_FragData[0] = texture2D(colortex0, uv);
    return;
#else

    float depth = texture2D(depthtex0, uv).r;
    if (depth < 0.9999) {
        gl_FragData[0] = texture2D(colortex0, uv);
        return;
    }

    vec3 sunDir = normalize(sunPosition);
    vec4 sp = gbufferProjection * (gbufferModelView * vec4(sunDir, 0.0));
    vec2 sunUV = sp.xy / sp.w * 0.5 + 0.5;
    sunUV = clamp(sunUV, vec2(0.0), vec2(1.0));

    vec2 toSun = sunUV - uv;

    vec3 viewDir = normalize(vec3(uv * 2.0 - 1.0, -1.0));
    float sunVis = clamp(dot(viewDir, sunDir), 0.0, 1.0);
    if (sunVis < 0.05) {
        gl_FragData[0] = texture2D(colortex0, uv);
        return;
    }

    float ditherOffset = hash12(gl_FragCoord.xy);

    vec2 stepUV = toSun / float(GODRAY_STEPS);
    vec2 cur    = uv + stepUV * ditherOffset;
    float accum = 0.0;

    for (int i = 0; i < GODRAY_STEPS; i++) {
        float d = texture2D(depthtex0, cur).r;
        if (d >= 0.9999) {
            vec3 s = texture2D(colortex0, cur).rgb;
            accum += luma(s);
        }
        cur += stepUV;
    }
    accum *= 1.0 / float(GODRAY_STEPS);
    accum *= GODRAY_STRENGTH;

    vec3 base = texture2D(colortex0, uv).rgb;
    gl_FragData[0] = vec4(base + GODRAY_COLOR * accum * sunVis, 1.0);
#endif
}
#endif
