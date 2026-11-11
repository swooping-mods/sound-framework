# Sound Framework: behavior and implementation

Describes the exported implementation **0.1.3**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Providers register immutable namespaced mono PCM16 cues and emit/move/stop spatial sounds through checked APIs on the observed Unity thread. Limits include 64 cues and 16 voices. Clips upload lazily or through explicit preparation; provider runtime assets use generation-checked handles. Finite position/pitch/gain bounds and per-cue/emitter cooldowns reject invalid work; saturation drops a new transient instead of evicting active cooldown ownership.

## State and lifecycle

Registry keys, voices, clips and emitters are process/world resources, not save data. Registration seals on first initialized world. Managed roots retain clips/sources/transforms; world changes stop voices and release owned scene resources. Existing game SFX volume/listener behavior is reused rather than adding a listener or rewriting native audio assets.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/sound-framework/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Provides `sound-framework.sound` | 1 | 48 bytes |
| Provides `sound-framework.sound-assets` | 1 | 32 bytes |
| Provides `sound-framework.sound-preparation` | 1 | 16 bytes |

## Implementation sources

- [mod.c](../source/mods/sound-framework/mod.c)
- [registry.h](../source/mods/sound-framework/registry.h)
- [voices.h](../source/mods/sound-framework/voices.h)
- [assets_registry.h](../source/mods/sound-framework/assets_registry.h)
- [clips.h](../source/mods/sound-framework/clips.h)
- [emit.h](../source/mods/sound-framework/emit.h)
- [preparation.h](../source/mods/sound-framework/preparation.h)

## Verification boundary

No network sound replication, automatic file decoder or global loudness guarantee. Providers supply valid audio and emitter lifetime; actual audibility, attenuation, device and game-mixer behavior require listening/native checks.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
