# Fish Frenzy — PlayStation 2 homebrew port

This is a controller-playable PS2 adaptation of [AhmedAbdoDev/Fish-Frenzy-ASU-Source-Code](https://github.com/AhmedAbdoDev/Fish-Frenzy-ASU-Source-Code). The original project is a Windows C++/SFML game, so its executable and SFML calls cannot run on a PS2 unchanged. This port reimplements the core arcade loop with PS2SDK/gsKit and includes a selected set of the original game's artwork, downsampled and embedded into the ELF.

## Imported art

- `assets/source/` holds the original title/background images and fish sprite sheets used by this port.
- `assets/converted/*.rgba` are PS2-ready power-of-two textures derived from those files. `src/assets.S` embeds them into the ELF, so the game does not need USB file access at runtime.
- `tools/convert_assets.py` reproduces the conversions (requires Pillow). Preview PNGs are included beside the raw textures.

The artwork was resized/cropped to fit PS2 video memory. The upstream music and sound effects are not yet ported; this version uses no audio. It also implements a compact version of the gameplay rather than reproducing every SFML menu, level, and feature.

## Controls

- D-pad: swim
- **R1**: short dash (recharges)
- **X**: start/retry
- **START**: pause/resume; on the game-over screen, return to title
- Eat smaller fish for points and gradual growth; avoid larger predators. You have three lives.

## Build the ELF

Requires a working **PS2SDK + gsKit** installation with the EE/MIPS R5900 toolchain. This workspace does not have that cross-toolchain installed, so there is **no compiled `.elf` in this ZIP**. A host-PC compiler cannot produce a PS2 executable.

On Linux/macOS with PS2DEV installed:

```sh
export PS2DEV=/usr/local/ps2dev
export PS2SDK="$PS2DEV/ps2sdk"
export GSKIT="$PS2DEV/gsKit"
export PATH="$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2SDK/bin"
make clean all
```

The expected output is `fishfrenzy.elf`. The Makefile links libpad and gsKit/dmaKit; image pixels are built in, not loaded from disk at runtime.

### GitHub Actions

The included `.github/workflows/build-ps2.yml` attempts a build in the `ps2dev/ps2dev` container and uploads `fishfrenzy.elf` as an Actions artifact. Push this folder's contents to a GitHub repository and open **Actions** to inspect the run. The workflow/container has not been executed in this workspace; if the image's installed paths or libraries differ, adjust the workflow/Makefile to match that PS2DEV image.

## Run on a PS2

This is a homebrew ELF, not a retail disc image. Launch it with a homebrew-capable setup (for example, Free McBoot/FreeDVDBoot plus uLaunchELF, or a compatible OPL ELF-launch workflow). Copy `fishfrenzy.elf` to FAT32 USB or another supported location, then launch it. Use a controller in port 1. Test in PCSX2 first if available. It has not been hardware-tested here.

## License / asset note

The upstream repository's MIT license is included in `LICENSE` and applies to its source code. The upstream README notes that third-party artwork/audio remain their respective creators' property; the included images are retained for this educational port. See the upstream repository for attribution and any asset-specific terms.
