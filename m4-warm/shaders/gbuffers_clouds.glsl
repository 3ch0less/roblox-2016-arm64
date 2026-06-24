#ifdef VERTEX_SHADER
varying vec2 vTexCoord;
varying vec4 vColor;
varying vec3 vNormal;

void main() {
    vec4 mv = gbufferModelView * vec4(position, 1.0);
    gl_Position = gbufferProjection * mv;
    vTexCoord = uv0;
    vColor    = color;
    vNormal   = normalize((gbufferModelViewInverse * vec4(normal, 0.0)).xyz);
}
#endif

#ifdef FRAGMENT_SHADER
/* DRAWBUFFERS:0 */

varying vec2 vTexCoord;
varying vec4 vColor;
varying vec3 vNormal;

uniform sampler2D gtexture;

void main() {
#if CLOUDS_ENABLED == 0
    discard;
    return;
#endif

    vec4 base = texture2D(gtexture, vTexCoord) * vColor;
    if (base.a < 0.05) discard;

    vec3 sunDir = normalize(sunPosition);
    float dayFactor = smoothstep(-0.05, 0.10, sunDir.y);

    vec3 lit = base.rgb;
    lit *= mix(0.62, 1.0, dayFactor);
    lit += vec3(0.95, 0.85, 0.70) * 0.20 * dayFactor;

#if CLOUDS_DETAIL == 1
    float h = vTexCoord.y;
    float band = sin(h * 80.0) * 0.04 + sin(h * 160.0 + 1.0) * 0.02;
    lit += band * dayFactor;
#endif

    gl_FragData[0] = vec4(lit, base.a);
}
#endif
