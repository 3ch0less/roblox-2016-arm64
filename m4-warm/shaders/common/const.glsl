const float PI       = 3.14159265359;
const float TAU      = 6.28318530718;
const float INV_PI   = 0.31830988618;
const float EPSILON  = 1e-5;

const float GAMMA    = 2.2;
const float INV_GAM  = 0.45454545455;

const vec3 GODRAY_COLOR = vec3(1.00, 0.86, 0.62);

#if M4WARM_PRESET == 0
    #define PRESET_POTATO 1
#elif M4WARM_PRESET == 1
    #define PRESET_LOW 1
#elif M4WARM_PRESET == 2
    #define PRESET_MEDIUM 1
#elif M4WARM_PRESET == 3
    #define PRESET_HIGH 1
#else
    #define PRESET_ULTRA 1
#endif

#if M4WARM_FOG == 0
    #define FOG_NEAR 1.00
    #define FOG_FAR  1.00
#elif M4WARM_FOG == 1
    #define FOG_NEAR 0.70
    #define FOG_FAR  0.96
#elif M4WARM_FOG == 2
    #define FOG_NEAR 0.55
    #define FOG_FAR  1.00
#else
    #define FOG_NEAR 0.40
    #define FOG_FAR  0.95
#endif

#if M4WARM_FOG_TINT == 0
    #define FOG_TINT_DAY   vec3(0.78, 0.80, 0.84)
    #define FOG_TINT_NIGHT vec3(0.55, 0.58, 0.62)
#elif M4WARM_FOG_TINT == 1
    #define FOG_TINT_DAY   vec3(0.78, 0.80, 0.78)
    #define FOG_TINT_NIGHT vec3(0.50, 0.52, 0.55)
#elif M4WARM_FOG_TINT == 2
    #define FOG_TINT_DAY   vec3(0.92, 0.86, 0.74)
    #define FOG_TINT_NIGHT vec3(0.70, 0.68, 0.65)
#elif M4WARM_FOG_TINT == 3
    #define FOG_TINT_DAY   vec3(0.95, 0.82, 0.60)
    #define FOG_TINT_NIGHT vec3(0.65, 0.55, 0.40)
#else
    #define FOG_TINT_DAY   vec3(0.92, 0.86, 0.74)
    #define FOG_TINT_NIGHT vec3(0.78, 0.80, 0.84)
#endif

#if M4WARM_SUN_INTENSITY == 0
    #define SUN_INTENSITY 0.85
#elif M4WARM_SUN_INTENSITY == 1
    #define SUN_INTENSITY 1.20
#else
    #define SUN_INTENSITY 1.55
#endif

#if M4WARM_MOON_INTENSITY == 0
    #define MOON_INTENSITY 0.00
#elif M4WARM_MOON_INTENSITY == 1
    #define MOON_INTENSITY 0.18
#else
    #define MOON_INTENSITY 0.35
#endif

#if M4WARM_AMBIENT == 0
    #define AMBIENT_FLOOR 0.10
#elif M4WARM_AMBIENT == 1
    #define AMBIENT_FLOOR 0.18
#else
    #define AMBIENT_FLOOR 0.26
#endif

#if M4WARM_WARM_TINT == 0
    #define WARM_TINT vec3(1.00, 1.00, 1.00)
#elif M4WARM_WARM_TINT == 1
    #define WARM_TINT vec3(1.02, 1.00, 0.97)
#elif M4WARM_WARM_TINT == 2
    #define WARM_TINT vec3(1.04, 1.00, 0.93)
#else
    #define WARM_TINT vec3(1.07, 1.00, 0.88)
#endif

#if M4WARM_SATURATION == 0
    #define SATURATION 0.60
#elif M4WARM_SATURATION == 1
    #define SATURATION 0.84
#else
    #define SATURATION 1.10
#endif

#if M4WARM_CONTRAST == 0
    #define CONTRAST_K 0.95
#elif M4WARM_CONTRAST == 1
    #define CONTRAST_K 1.04
#else
    #define CONTRAST_K 1.15
#endif

#if M4WARM_VIBRANCE == 0
    #define VIBRANCY 0.00
#elif M4WARM_VIBRANCE == 1
    #define VIBRANCY 0.06
#else
    #define VIBRANCY 0.12
#endif

#if M4WARM_VIGNETTE == 0
    #define VIGNETTE_K 0.00
#elif M4WARM_VIGNETTE == 1
    #define VIGNETTE_K 0.25
#else
    #define VIGNETTE_K 0.50
#endif

#if M4WARM_EXPOSURE == 0
    #define EXPOSURE 0.85
#elif M4WARM_EXPOSURE == 1
    #define EXPOSURE 1.00
#else
    #define EXPOSURE 1.20
#endif

#if M4WARM_GODRAYS == 0
    #define GODRAY_ENABLED 0
#elif M4WARM_GODRAYS == 1
    #define GODRAY_ENABLED 1
    #define GODRAY_STRENGTH 0.18
#else
    #define GODRAY_ENABLED 1
    #define GODRAY_STRENGTH 0.32
#endif

#if M4WARM_GODRAY_QUALITY == 0
    #define GODRAY_STEPS 4
#elif M4WARM_GODRAY_QUALITY == 1
    #define GODRAY_STEPS 8
#else
    #define GODRAY_STEPS 16
#endif

#if M4WARM_RT_MODE == 0
    #define RT_ENABLED 0
#elif M4WARM_RT_MODE == 1
    #define RT_ENABLED 1
    #define RT_SSR_ON 1
    #define RT_AO_ON 0
#else
    #define RT_ENABLED 1
    #define RT_SSR_ON 1
    #define RT_AO_ON 1
#endif

#if M4WARM_RT_MODE == 2
    #define RT_SSR_STEPS 24
    #define RT_SSR_MAX_DIST 120.0
    #define RT_SSR_INTENSITY 0.85
    #define RT_SSR_HALF_RES 0
    #define RT_AO_DIRS 8
    #define RT_AO_STEPS 6
    #define RT_AO_RADIUS 0.80
    #define RT_AO_INTENSITY 0.50
#endif

#if M4WARM_SSR_STEPS == 0
    #ifdef PRESET_POTATO
        #define RT_SSR_STEPS 6
    #elif defined(PRESET_LOW)
        #define RT_SSR_STEPS 8
    #elif defined(PRESET_MEDIUM)
        #define RT_SSR_STEPS 12
    #elif defined(PRESET_HIGH)
        #define RT_SSR_STEPS 16
    #else
        #define RT_SSR_STEPS 24
    #endif
#elif M4WARM_SSR_STEPS == 1
    #define RT_SSR_STEPS 6
#elif M4WARM_SSR_STEPS == 2
    #define RT_SSR_STEPS 8
#elif M4WARM_SSR_STEPS == 3
    #define RT_SSR_STEPS 12
#elif M4WARM_SSR_STEPS == 4
    #define RT_SSR_STEPS 16
#else
    #define RT_SSR_STEPS 24
#endif

#if M4WARM_SSR_DISTANCE == 0
    #define RT_SSR_MAX_DIST 25.0
#elif M4WARM_SSR_DISTANCE == 1
    #define RT_SSR_MAX_DIST 60.0
#else
    #define RT_SSR_MAX_DIST 120.0
#endif

#if M4WARM_SSR_STRENGTH == 0
    #define RT_SSR_INTENSITY 0.0
#elif M4WARM_SSR_STRENGTH == 1
    #define RT_SSR_INTENSITY 0.30
#elif M4WARM_SSR_STRENGTH == 2
    #define RT_SSR_INTENSITY 0.55
#else
    #define RT_SSR_INTENSITY 0.85
#endif

#if M4WARM_SSR_RES == 0
    #define RT_SSR_HALF_RES 1
#else
    #define RT_SSR_HALF_RES 0
#endif

#if M4WARM_AO_DIRS == 0
    #ifdef PRESET_POTATO
        #define RT_AO_DIRS 4
    #elif defined(PRESET_LOW)
        #define RT_AO_DIRS 4
    #elif defined(PRESET_MEDIUM)
        #define RT_AO_DIRS 6
    #else
        #define RT_AO_DIRS 8
    #endif
#elif M4WARM_AO_DIRS == 1
    #define RT_AO_DIRS 4
#elif M4WARM_AO_DIRS == 2
    #define RT_AO_DIRS 6
#else
    #define RT_AO_DIRS 8
#endif

#if M4WARM_AO_STEPS == 0
    #ifdef PRESET_POTATO
        #define RT_AO_STEPS 2
    #elif defined(PRESET_LOW)
        #define RT_AO_STEPS 3
    #else
        #define RT_AO_STEPS 4
    #endif
#elif M4WARM_AO_STEPS == 1
    #define RT_AO_STEPS 2
#elif M4WARM_AO_STEPS == 2
    #define RT_AO_STEPS 3
#elif M4WARM_AO_STEPS == 3
    #define RT_AO_STEPS 4
#else
    #define RT_AO_STEPS 6
#endif

#if M4WARM_AO_RADIUS == 0
    #define RT_AO_RADIUS 0.25
#elif M4WARM_AO_RADIUS == 1
    #define RT_AO_RADIUS 0.45
#else
    #define RT_AO_RADIUS 0.80
#endif

#if M4WARM_AO_STRENGTH == 0
    #define RT_AO_INTENSITY 1.00
#elif M4WARM_AO_STRENGTH == 1
    #define RT_AO_INTENSITY 0.85
#elif M4WARM_AO_STRENGTH == 2
    #define RT_AO_INTENSITY 0.70
#else
    #define RT_AO_INTENSITY 0.50
#endif

#if M4WARM_SHADOWS == 0
    #define SHADOW_ENABLED 0
#elif M4WARM_SHADOWS == 1
    #define SHADOW_ENABLED 1
    #define SHADOW_PCF_TAPS 4
#else
    #define SHADOW_ENABLED 1
    #define SHADOW_PCF_TAPS 9
#endif

#if M4WARM_SKY_BRIGHTNESS == 0
    #define SKY_BRIGHTNESS 0.75
#elif M4WARM_SKY_BRIGHTNESS == 1
    #define SKY_BRIGHTNESS 1.00
#else
    #define SKY_BRIGHTNESS 1.25
#endif

#if M4WARM_STARS == 0
    #define STARS_INTENSITY 0.00
#elif M4WARM_STARS == 1
    #define STARS_INTENSITY 0.05
#else
    #define STARS_INTENSITY 0.12
#endif

#if M4WARM_CLOUDS == 0
    #define CLOUDS_ENABLED 0
#elif M4WARM_CLOUDS == 1
    #define CLOUDS_ENABLED 1
    #define CLOUDS_DETAIL 0
#else
    #define CLOUDS_ENABLED 1
    #define CLOUDS_DETAIL 1
#endif
