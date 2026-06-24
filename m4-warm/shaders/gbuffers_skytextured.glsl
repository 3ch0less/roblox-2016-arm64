#ifdef VERTEX_SHADER
varying vec2 vTexCoord;
varying vec3 vWorldDir;

void main() {
    vec4 mv = gbufferModelView * vec4(position, 1.0);
    gl_Position = gbufferProjection * mv;
    vTexCoord = uv0;
    vWorldDir = normalize(position);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:0 */

varying vec2 vTexCoord;
varying vec3 vWorldDir;

uniform sampler2D gtexture;

void main() {
    vec4 c = texture2D(gtexture, vTexCoord);
    if (c.a < 0.5) discard;
    gl_FragData[0] = vec4(c.rgb, 1.0);
}
#endif
