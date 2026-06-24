#ifdef VERTEX_SHADER
void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:0 */

uniform sampler2D colortex0;
uniform sampler2D colortex2;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(viewWidth, viewHeight);
    vec3 godrayed = texture2D(colortex2, uv).rgb;

    float vig = 1.0 - VIGNETTE_K * pow(length(uv - 0.5) * 1.414, 2.2);
    godrayed *= vig;

    gl_FragData[0] = vec4(godrayed, 1.0);
}
#endif
