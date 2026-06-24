#ifdef VERTEX_SHADER
void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER

uniform sampler2D colortex0;
uniform sampler2D colortex1;
uniform sampler2D depthtex0;
uniform sampler2D depthtex1;
uniform float eyeBrightness;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(viewWidth, viewHeight);

    vec3 scene = texture2D(colortex0, uv).rgb;

#if M4WARM_WATER == 0
    vec3 merged = scene;
#else
    float dOpaque = texture2D(depthtex0, uv).r;
    float dTrans  = texture2D(depthtex1, uv).r;
    vec4 waterSample = texture2D(colortex1, uv);
    vec3  water      = waterSample.rgb;
    float waterA     = waterSample.a;

    vec3 merged = scene;
    if (waterA > 0.001 && (dTrans < dOpaque || dOpaque >= 0.9999)) {
        merged = mix(scene, water, waterA);
    }
#endif

    merged *= EXPOSURE;
    merged *= clamp(eyeBrightness * 1.0 + (1.0 - 0.4 * eyeBrightness), 0.6, 1.0);

    vec3 mapped = applyTonemap(merged);
    mapped = applyGrade(mapped);
    mapped = toSrgb(mapped);

    gl_FragColor = vec4(mapped, 1.0);
}
#endif
