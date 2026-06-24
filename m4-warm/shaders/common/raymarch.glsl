vec3 reconstructViewPos(vec2 uv, float depth) {
    vec3 clip = vec3(uv * 2.0 - 1.0, depth * 2.0 - 1.0);
    vec4 cp = gbufferProjectionInverse * vec4(clip, 1.0);
    return cp.xyz / cp.w;
}

vec3 decodeNormal(vec3 n) {
    return n * 2.0 - 1.0;
}

vec3 encodeNormal(vec3 n) {
    return n * 0.5 + 0.5;
}

void tangentBasis(vec3 n, out vec3 t, out vec3 b) {
    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

float rtao(vec2 uv, vec3 viewPos, vec3 viewNormal) {
    float depth = texture2D(depthtex0, uv).r;
    if (depth >= 0.9999) return 1.0;

    vec3 t, b;
    tangentBasis(viewNormal, t, b);

    float occ = 0.0;
    int total = 0;

    for (int i = 0; i < RT_AO_DIRS; i++) {
        float a = float(i) * 1.04719755;
        vec2 dir2D = vec2(cos(a), sin(a));
        vec3 dir = normalize(dir2D.x * t + dir2D.y * b);

        for (int j = 0; j < RT_AO_STEPS; j++) {
            float stepDist = (float(j) + 1.0) / float(RT_AO_STEPS) * RT_AO_RADIUS;
            vec3 samplePos = viewPos + dir * stepDist;

            vec4 proj = gbufferProjection * vec4(samplePos, 1.0);
            vec2 sampleUV = proj.xy / proj.w * 0.5 + 0.5;

            total++;

            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;

            float sceneDepth = texture2D(depthtex0, sampleUV).r;
            if (sceneDepth >= 0.9999) continue;

            vec3 scenePos = reconstructViewPos(sampleUV, sceneDepth);
            float actualDist = length(scenePos - viewPos);

            if (actualDist < stepDist - 0.04) {
                float amount = (stepDist - actualDist) / stepDist;
                occ += amount * amount;
            }
        }
    }

    if (total == 0) return 1.0;
    occ /= float(total);
    return clamp(1.0 - occ * 2.4, 0.0, 1.0);
}

vec3 ssr(vec2 uv, vec3 viewPos, vec3 viewNormal, float roughness, float depth) {
    if (roughness > 0.65) return vec3(0.0);
    if (depth >= 0.9999) return vec3(0.0);

    vec3 viewDir = normalize(viewPos);
    vec3 reflDir = reflect(viewDir, viewNormal);

    if (reflDir.z > -0.05) return vec3(0.0);

    vec3 stepVec = reflDir * (RT_SSR_MAX_DIST / float(RT_SSR_STEPS));
    float dither = hash12(gl_FragCoord.xy);

    vec3 hitColor = vec3(0.0);
    float hitMask = 0.0;

    for (int i = 1; i <= RT_SSR_STEPS; i++) {
        vec3 samplePos = viewPos + stepVec * (float(i) + dither);
        vec4 proj = gbufferProjection * vec4(samplePos, 1.0);
        vec2 sampleUV = proj.xy / proj.w * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) break;

        float sceneDepth = texture2D(depthtex0, sampleUV).r;
        if (sceneDepth >= 0.9999) continue;

        vec3 scenePos = reconstructViewPos(sampleUV, sceneDepth);
        float rayT  = length(samplePos - viewPos);
        float depthT = length(scenePos - viewPos);

        if (depthT < rayT) {
            hitColor = texture2D(colortex0, sampleUV).rgb;
            float fade = 1.0 - smoothstep(0.0, 0.6, rayT - depthT);
            float roughFade = 1.0 - roughness;
            hitMask = fade * roughFade;
            break;
        }
    }

    return hitColor * hitMask;
}

vec3 bilateralFilter(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 center = texture2D(tex, uv).rgb;
    float centerDepth = texture2D(depthtex0, uv).r;
    vec3 centerNorm = decodeNormal(texture2D(gaux1, uv).rgb);

    vec3 sum = center;
    float weight = 1.0;

    for (int i = 0; i < 4; i++) {
        vec2 offset = (i == 0) ? vec2( 1.0, 0.0) :
                      (i == 1) ? vec2(-1.0, 0.0) :
                      (i == 2) ? vec2( 0.0, 1.0) :
                                 vec2( 0.0,-1.0);
        vec2 sampleUV = uv + offset * texelSize;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;

        vec3 sample = texture2D(tex, sampleUV).rgb;
        float sampleDepth = texture2D(depthtex0, sampleUV).r;
        vec3 sampleNorm = decodeNormal(texture2D(gaux1, sampleUV).rgb);

        float depthDiff = abs(sampleDepth - centerDepth);
        float normDiff  = 1.0 - max(0.0, dot(sampleNorm, centerNorm));
        float w = exp(-depthDiff * 40.0 - normDiff * 8.0);
        weight += w;
        sum += sample * w;
    }

    if (weight < 0.001) return center;
    return sum / weight;
}
