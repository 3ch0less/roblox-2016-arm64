#ifdef VERTEX_SHADER
varying vec4 vColor;

void main() {
    vec4 mv = gbufferModelView * vec4(position, 1.0);
    gl_Position = gbufferProjection * mv;
    vColor = color;
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:0 */

varying vec4 vColor;

void main() {
    float g = mix(0.55, 0.78, vColor.r);
    gl_FragData[0] = vec4(vec3(g), 0.6);
}
#endif
