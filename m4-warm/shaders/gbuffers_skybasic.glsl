#ifdef VERTEX_SHADER
varying vec3 vViewDir;
varying vec3 vWorldDir;

void main() {
    vec4 mv = gbufferModelView * vec4(position, 1.0);
    gl_Position = gbufferProjection * mv;
    vViewDir  = normalize(-mv.xyz);
    vWorldDir = normalize(position);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:0 */

varying vec3 vViewDir;
varying vec3 vWorldDir;

void main() {
    vec3 d = normalize(vWorldDir);
    float h = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizon = vec3(0.78, 0.74, 0.66);
    vec3 zenith  = vec3(0.40, 0.55, 0.74);
    vec3 nadir   = vec3(0.62, 0.55, 0.45);
    vec3 sky = (h > 0.5)
        ? mix(horizon, zenith, smoothstep(0.5, 1.0, h))
        : mix(nadir,   horizon, smoothstep(0.0, 0.5, h));

    vec3 sunDir = normalize(sunPosition);
    float sunDot = max(0.0, dot(d, sunDir));
    sky += vec3(1.05, 0.92, 0.72) * pow(sunDot, 8.0) * 0.35 * SUN_INTENSITY;

    float moonDot = max(0.0, dot(d, normalize(moonPosition)));
    sky += vec3(0.80, 0.85, 0.95) * pow(moonDot, 64.0) * 0.55 * MOON_INTENSITY;

    float starsMask = smoothstep(0.45, 0.85, h) * (1.0 - smoothstep(-0.05, 0.10, sunDir.y));
    sky += vec3(0.85, 0.88, 0.95) * starsMask * STARS_INTENSITY;

    sky *= SKY_BRIGHTNESS;
    sky += dither8(gl_FragCoord.xy);

    gl_FragData[0] = vec4(sky, 1.0);
}
#endif
