float pcf2x2(sampler2D shadowMap, vec3 sc, float bias) {
    vec2 t = 1.0 / vec2(textureSize(shadowMap, 0));
    float s = 0.0;
    s += step(sc.z - bias, texture2D(shadowMap, sc.xy + vec2( 0.0, 0.0) * t).r);
    s += step(sc.z - bias, texture2D(shadowMap, sc.xy + vec2( 1.0, 0.0) * t).r);
    s += step(sc.z - bias, texture2D(shadowMap, sc.xy + vec2( 0.0, 1.0) * t).r);
    s += step(sc.z - bias, texture2D(shadowMap, sc.xy + vec2( 1.0, 1.0) * t).r);
    return s * 0.25;
}
