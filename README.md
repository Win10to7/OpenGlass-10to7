![header](assets/banner.png)

# Experience the native Aero Glass interface on Windows 10+

OpenGlass restores the full glass effect to window frames, with control over blur, reflections, colorization, caption rendering, and theme integration.

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/ALTaleX531/OpenGlass)
[![CI](https://github.com/ALTaleX531/OpenGlass/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/ALTaleX531/OpenGlass/actions/workflows/build.yml)

## Supported Windows versions

| Windows build | Status |
| --- | --- |
| Windows 10 build 17763 through 19045 | Stable |
| Windows 11 builds below 28000, including build 26200 | Stable |
| Windows 11 build 28000 and later | Experimental |
| Windows Server 2022 | Supported |

Only General Availability Windows builds are supported. Insider and preview builds, and Windows Server versions other than 2022, are unsupported and may crash DWM. Compatibility depends on the exact build, revision, and verified compositor capabilities.

See [Compatibility and DWM architectures](https://github.com/ALTaleX531/OpenGlass/wiki/Compatibility-and-DWM-architectures) for the complete support policy and explanation of parallel Windows build trains.

## Quick start

1. Download `OpenGlassSetup.exe` from [Releases](https://github.com/ALTaleX531/OpenGlass/releases).
2. Install OpenGlass and open its GUI. The GUI requests administrator elevation because it manages both per-user Windows colorization and system-wide OpenGlass settings. The [configuration reference](https://github.com/ALTaleX531/OpenGlass/wiki/Configuration-and-registry-reference) documents the corresponding registry values.
3. Adjust the appearance. Changes apply immediately; **Save** accepts the current state and **Revert** restores the state captured before editing.

Current releases use one unified `OpenGlassSetup.exe`. Older OpenGlass releases required users to choose a build-specific installer for their Windows version; that manual selection is no longer needed. The unified installer detects the Windows build and installs only the matching DWM implementation.

The **Glass colors** page includes Windows Vista and Windows 7 presets. The **Preset packs** page can import, create, apply, and remove immutable [preset ZIPs](https://github.com/ALTaleX531/OpenGlass/wiki/Preset-packages). The official GUI and preset packages manage one system-wide configuration for effects and themes; only the five Windows colorization values and their Override forms are written per-user. The OpenGlass runtime remains compatible with manual and transformation-pack settings in either HKCU or HKLM.

> [!TIP]
> **Emergency Exit:** Long-press <kbd>Ctrl</kbd>+<kbd>Win</kbd>+<kbd>Shift</kbd>+<kbd>Q</kbd> to terminate DWM if the system becomes unresponsive.

OpenGlass is intended for advanced users who are comfortable troubleshooting DWM. For a simpler alternative, consider [DWMBlurGlass](https://github.com/Maplespe/DWMBlurGlass).

## Reporting issues

A DWM crash is a bug report, not a symbol-download problem. Open a [GitHub issue](https://github.com/ALTaleX531/OpenGlass/issues/new) promptly; posts on Reddit, Discord, or other third-party communities are not tracked as OpenGlass bug reports. For unexpectedly opaque glass, first check the GUI's **Diagnostics** tab, which reports the Windows transparency setting, opaque-blend setting, effective power mode, and battery-saver policy. For a DWM crash or hang, enable full DWM dumps there and reproduce the problem once. Include the exact Windows build and revision, OpenGlass version, registry settings, reproduction steps, screenshots or recordings, and a dump when a crash occurred. See [Troubleshooting and crash dumps](https://github.com/ALTaleX531/OpenGlass/wiki/Troubleshooting-and-crash-dumps).

## Building

```powershell
msbuild OpenGlass.slnx /m /restore /p:Configuration=Release /p:Platform=x64
```

The `main` branch is also built and tested by GitHub Actions. Its downloadable `v<version>-unsigned` artifact is an unsigned validation build, not a release or Git tag. See [Building OpenGlass](https://github.com/ALTaleX531/OpenGlass/wiki/Building-OpenGlass) for prerequisites, output paths, packaging, tests, CI behavior, and signing requirements.

**Registry locations**:

- `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\DWM` (per-user, checked first)
- `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\DWM` (system-wide fallback)

**Key inheritance**: Missing keys use predefined defaults. Variants (e.g., `XXXInactive`, `XXXMaximized`) inherit from their base key if not explicitly set. Keys with the `Override` suffix take precedence and resist resets by `uxtheme.dll` on Windows 10+.

## Registry reference

### Colorization settings

| Key Name | Type | Description |
| -------- | ---- | ----------- |
| ColorizationColor(Override)<br>ColorizationColorInactive<br>ColorizationAfterglow(Override) | DWORD | ARGB color used for the glass effect, alpha channel is ignored.<br><br>ℹ️ `ColorizationColorInactive` is only used when `GlassType` = 0x0<br>ℹ️ `ColorizationAfterglow(Override)` is only used when `GlassType` = 0x1 |
| ColorizationColorBalance(Override)<br>ColorizationAfterglowBalance(Override)<br>ColorizationBlurBalance(Override) | DWORD | Composition parameters for Windows 7 Aero effect shader.<br><br>ℹ️ Only used when `GlassType` = 0x1 |
| GlassOpacity<br>GlassOpacityInactive | DWORD | The intensity of the color (0-100%). Default value is 63%.<br><br>ℹ️ Only used when `GlassType` = 0x0 |
| ColorizationColorCaption<br>ColorizationColorCaptionInactive<br>ColorizationColorCaptionMaximized<br>ColorizationColorCaptionInactiveMaximized | DWORD | Color used for drawing window titles. Format is 0xBBGGRR.<br><br><ul><li>0xFFFFFFFF = Determined by the system</li><li>0xFFFFFFFE = Read the `TEXTCOLOR` property from the current theme to obtain them.</li><li>0xFFFFFFFD = Automatically select the appropriate text colors based on `GlassType`. (default)</li></ul> |
| ColorizationOpaqueBlend | DWORD | Controls the transparency of glass effect (default = 0). |
| ColorizationBaseTransparent<br>ColorizationBaseMaximized<br>ColorizationBaseOpaque | DWORD | ARGB base color used for color blending. <br><br><ul><li>0xFFFFFFFE = Automatically select the appropriate base color based on `GlassType` (default).</li><li>0xFFFFFFFF = Read the `COLORIZATIONCOLOR` property from the current theme to obtain them.</li></ul> |
| ColorizationOpaqueBlendPriority | DWORD | Behavior of choosing opaque blend base color. <br><br><ul><li>0x0 = Windows Vista.</li><li>0x1 = Windows 7.</li><li>0xFFFFFFFF = Automatically select the appropriate behavior based on `GlassType` (default).</li></ul>ℹ️ For Windows Vista, `ColorizationBaseMaximized` is preferred, whereas for Windows 7 it is `ColorizationBaseOpaque`. |
| ColorizationOpacity<br>ColorizationOpacityInactive<br>ColorizationOpacityMaximized<br>ColorizationOpacityInactiveMaximized | DWORD | (Additional) factors applied to glass color blending. (0%-100%). <br><br><ul><li>0xFFFFFFFE = Automatically select the appropriate factors based on `GlassType`. (default).</li><li>0xFFFFFFFF = Read the `COLORIZATIONOPACITY` property from the current theme to obtain them.</li></ul> |

### Glass settings

| Key Name | Type | Description |
| -------- | ---- | ----------- |
| GlassType | DWORD | The type of glass effect. <br><br><ul><li>0x0 = Windows Vista style blur (default).</li><li>0x1 = Windows 7 style blur.</li></ul> |
| GlassOverrideAccent | DWORD | Overrides accent blur surfaces with OpenGlass glass effects (e.g. the win10 taskbar). Default is 0. |
| CustomThemeReflection | String | Path to file with texture that is stretched over whole desktop and rendered above glass regions (default is Aero Glass Win7 reflection texture) |
| ColorizationGlassReflectionIntensity | DWORD | The overall multiplier applied to the intensity of reflection effect (0-100%). Default value is 0%.<br><br>opacity = base_opacity * intensity * 2 |
| ColorizationGlassReflectionOpacity<br>ColorizationGlassReflectionOpacityInactive<br>ColorizationGlassReflectionOpacityMaximized<br>ColorizationGlassReflectionOpacityInactiveMaximized | DWORD | The base opacity of reflection effect (0-100%). <br><br><ul><li>0xFFFFFFFE = Automatically select the appropriate factors based on `GlassType`. (default)</li><li>0xFFFFFFFF = Read the `OPACITY` property of `SQUEEGEREFLECTIONMAP` from the current theme to obtain them.</li></ul> |
| ColorizationGlassReflectionParallaxIntensity | DWORD | The parallax intensity of the reflection effect (e.g. when moving the windows side to side). Default value is 13%. |
| ColorizationGlassReflectionPolicy | DWORD | Controls where reflections should be rendered (default = 0xFFFFFFFF). <br><br><ul><li>Titlebar = 1<<0</li><li>Aero Peek = 1<<2</li><li>Aero Snap = 1<<3 (ℹ️ Only effective in Win10)</li><li>Render everywhere if possible = 0xFFFFFFFF</li></ul> |
| BlurDeviation | DWORD | Standard deviation for gaussian blur, default = 30 (which means σ = 3.0) <br>Value 0 results in non-blurred transparency.<br><br>ℹ️ Only effective when `UseDirect3DRendering` = 0x0 |
| BlurOptimization | DWORD | Quality of gaussian blur<br><br><ul><li>0x0 = Speed first (default)</li><li>0x1 = Balance</li><li>0x2 = Quality first</li></ul>  |
| RoundRectRadius | DWORD | The radius of glass geometry (default = 0), Win8=0, Win7=6 |
| CustomThemeMaterial | String | Path to file with texture that is rendered (tiled) above glass regions (default is Acrylic noise texture) |
| MaterialOpacity | DWORD | opacity of material texture (default = 0) |
| UseDirect3DRendering | DWORD | Set 1 to use d3d11 as glass renderer backend, and the blur radius is hardcoded to 3. (default = 0) |

### Theme settings

| Key Name | Type | Description |
| -------- | ---- | ----------- |
| CaptionButtons | DWORD | Changes caption buttons sizes, icon left margin and the opacity of the button glyphs.<br><br><ul><li>0x0 = Vanilla style (default)</li><li>0x1 = Windows Vista style</li><li>0x2 = Windows 7 style</li><li>0x3 = Windows 8 style</li></ul> |
| CenterCaption | DWORD | Controls how title bar text is aligned.<br><br><ul><li>0x0 = Keeps it on the left (default)</li><li>0x1 = Regular centering</li><li>0x2 = Windows 8 style centering</li></ul> |
| CaptionTextAliasing | DWORD | Selects the title bar text rasterizer.<br><br><ul><li>0x0 = Windows 7 style (default). Uses ClearType when system font smoothing is enabled and grayscale antialiasing when it is disabled.</li><li>0x1 = Windows 8 style (heavier stems, stronger colour fringing)</li></ul> |
| TextGlowMode | DWORD | Specifies how window caption glow effect will be rendered <br><br><ul><li>0x0 = No glow effect</li><li>0x1 = Glow effect loaded from atlas (default)</li><li>0x2 = Glow effect loaded from atlas and theme opacity is respected</li><li>0x3 = Composited glow effect using your theme settings HIWORD of the value specifies glow size (0 = theme default)</li></ul> |
| CustomThemeAtlas | String | Path to PNG file with theme resource (bitmap must have exactly the same layout as msstyle theme you are using!). <br><br>💡 OpenGlass also looks for a `.layout` file with the same name (e.g., `theme.png.layout`) to determine the layout of the atlas. |
| DisableModernBorders | DWORD | Disable modern rounded window borders. <br><br><ul><li>0x0 = Enable modern borders (default)</li><li>0x1 = Disable modern borders</li></ul><br>ℹ️ Only effective in Win11 |

### Advanced settings

These settings are intended for `HKLM` and should only be modified if necessary.

> [!CAUTION]
> Do not modify this section unless you fully understand the impact.


| Key Name | Type | Description |
| -------- | ---- | ----------- |
| DisableGlassOnBattery | DWORD | <ul><li>0x1 = When energy saver is on then the glass effect will be opaque to decrease energy consumption (default)</li><li>0x0 = glass effect won't be opaque on energy saver</li></ul> |
| DisabledHooks | DWORD | Controls which module's hooks are disabled, which will also control the availability of features. <br><br><ul><li>0x0 = No hooks are disabled (default)</li><li>0x1 = Disables hooks for [CaptionTextHandler.cpp](OpenGlass/CaptionTextHandler.cpp)</li><li>0x2 = Disables hooks for [AccentOverrider.cpp](OpenGlass/AccentOverrider.cpp)</li><li>0x4 = Disables hooks for [GlassFrameHandler.cpp](OpenGlass/GlassFrameHandler.cpp)</li><li>0x8 = Disables hooks for [GlassReflectionHandler.cpp](OpenGlass/GlassReflectionHandler.cpp)</li><li>0x10 = Disables hooks for [CaptionMetricsTweaker.cpp](OpenGlass/CaptionMetricsTweaker.cpp)</li></ul><br>⚠️ Should only be used to maintain compatibility with third-party applications. |
| GlassSafetyZoneMode | DWORD | Set 0 to disable glass safety zone. (default = 1) |

## Credits

- [Banner for OpenGlass](https://github.com/ALTaleX531/OpenGlass/discussions/11) by [@aubymori](https://github.com/aubymori), using [metalheart jawn #2](https://www.deviantart.com/kfh83/art/metalheart-jawn-2-1068250045) by [@kfh83](https://github.com/kfh83)
- [[MS-RDPCR2]: Remote Desktop Protocol: Composited Remoting V2](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpcr2)
- [KNSoft.SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours)
- [VC-LTL](https://github.com/Chuyu-Team/VC-LTL5)
- [Windows Implementation Libraries](https://github.com/Microsoft/wil)
- [libvalinet](https://github.com/valinet/libvalinet), whose symbol download work inspired OpenGlass
- [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB), whose C++ project structure inspired OpenGlass

## Support

OpenGlass is developed in free time and distributed under the GPLv3 license. DWM does not officially support extensibility, so future Windows updates may cause breakage and continuous support cannot be guaranteed.

If you find OpenGlass valuable, please consider supporting the project via Ko-fi. Donations are voluntary, carry no expectation of consideration, and must be made as a natural person.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/altalex531)
