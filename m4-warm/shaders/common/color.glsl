float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 saturation(vec3 c, float s) {
    float l = luma(c);
    return mix(vec3(l), c, s);
}

vec3 vibrance(vec3 c, float v) {
    float l = luma(c);
    float mx = max(max(c.r, c.g), c.b);
    return mix(c, c + (c - vec3(l)) * (1.0 - mx) * v, 1.0);
}

vec3 contrast(vec3 c, float k) {
    return (c - 0.5) * k + 0.5;
}

vec3 warmShift(vec3 c) {
    return c * WARM_TINT;
}

vec3 applyGrade(vec3 c) {
    c = vibrance(c, VIBRANCY);
    c = saturation(c, SATURATION);
    c = warmShift(c);
    c = contrast(c, CONTRAST_K);
    return clamp(c, 0.0, 1.0);
}

vec3 toSrgb(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(INV_GAM));
}

vec3 fromSrgb(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(GAMMA));
}
