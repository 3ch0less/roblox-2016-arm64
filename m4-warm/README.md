# M4-Warm

Shader pack tuned for MacBook Air M4 / Pro / Max, Metal 3, 16 GB unified memory.

## Install

1. Fabric 0.14+ · Sodium 0.5.8+ · Iris 1.6.17+
2. Drop the `m4-warm/` folder into `~/.minecraft/shaderpacks/`
3. Options → Video Settings → Shader Packs → **M4-Warm** → click the gear icon
4. Restart the world once after first install so Iris re-creates its buffers

## Settings (gear icon → 30+ options, grouped)

- **General**: Quality Preset (Potato / Low / Medium / High / Ultra)
- **Lighting**: Sun / Moon / Ambient / Exposure
- **Atmosphere**: Fog distance + tint, God Rays (strength + quality)
- **Color Grading**: Tonemap, Warm Tint, Saturation, Contrast, Vibrance, Vignette
- **Shadows**: Soft (2x2 PCF) / Softer (3x3 PCF) / Off
- **Water**: Effect, Quality
- **Ray Tracing (Screen-Space)**: SSR + RTAO, with individual sliders for steps, distance, strength, resolution, AO directions, AO steps, AO radius, AO strength
- **Sky & Clouds**: Sky brightness, stars, clouds
- **Performance**: Internal render scale (0.5x / 0.75x / 1.0x)

Every option cascades from the Quality Preset. Pick a tier, then override individual settings as needed.

## Presets

| Preset   | Best for               | AO dirs | SSR steps | SSR res | Shadow |
|----------|------------------------|---------|-----------|---------|--------|
| Potato   | 8 GB, FPS first       | 4       | 6         | Half    | Off    |
| Low      | 8 GB, balance          | 4       | 8         | Half    | Soft   |
| Medium   | 16 GB (default)        | 6       | 12        | Half    | Soft   |
| High     | 16 GB+                 | 8       | 16        | Half    | Soft   |
| Ultra    | 24 GB+                 | 8       | 24        | Full    | Softer |

## Pipeline

1. `gbuffers_*` write lit color to `colortex0` and normal+roughness to `gaux1` (water → `colortex1` only)
2. `composite` marches `GODRAY_STEPS` (4/8/16) dithered steps toward the sun for god rays → `colortex2`
3. `composite1` merges god rays + applies vignette → `colortex0`
4. `composite2` does SSR (`RT_SSR_STEPS`) + RTAO (`RT_AO_DIRS × RT_AO_STEPS`) at half or full res → `colortex3`
5. `composite3` bilateral-denoises `colortex3`, upscales, applies AO + reflections to `colortex0`
6. `final` blends `colortex1` (water) over `colortex0`, applies tonemap + color grade + sRGB encode → screen

## Ray tracing

All "raytracing" in this pack is screen-space (same technique used by SEUS PTGI, BSL, Complementary):

- **SSR**: raymarch against `depthtex0`, reconstruct view position, compare depths. CryEngine 3 method.
- **RTAO**: `RT_AO_DIRS` fixed tangent-space directions × `RT_AO_STEPS` decreasing step distances, GTAO-style.
- **Denoise**: 5-tap bilateral cross filter weighted by depth + normal difference.
- **Dithered**: per-pixel hash offset on march start breaks banding without needing a temporal history buffer.

Hardware RT (Apple M4 supports it via Metal 3 `intersection_query`) is **not used**, Minecraft is voxel-based and Iris doesn't expose the voxel grid.

## Tuning

Most knobs are in `shaders/common/const.glsl`. The lang file is in `shaders/lang/en_US.lang` the in-game settings panel maps directly to those options.

## Limitations

- SSR can't reflect off-screen geometry.
- RTAO has typical SSAO halos at depth discontinuities; bilateral filter reduces them.
- No real bounce light between surfaces.
- No temporal accumulation; dithered blue-noise used instead.
- No hardware RT cores engaged.
- Water has its own simple screen-space reflection in `gbuffers_water` (runs after composite), separate from SSR.
