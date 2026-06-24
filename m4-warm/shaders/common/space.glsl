vec3 reconstructViewPos(vec2 uv, float depth) {
    vec3 clip = vec3(uv * 2.0 - 1.0, depth * 2.0 - 1.0);
    vec4 cp = gbufferProjectionInverse * vec4(clip, 1.0);
    return cp.xyz / cp.w;
}

float linearizeDepth(float d) {
    return (2.0 * near) / (far + near - d * (far - near));
}
