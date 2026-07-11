# AnimeWwise FUSE (`fuseaw`)

[FUSE](https://github.com/libfuse/libfuse) binding for extracting audio from some anime games.

Concepts taken and code derived from [AnimeWwise](https://github.com/Escartem/AnimeWwise).

## Compile

```bash
g++ -std=c++23 fuseaw.cpp src/*.cpp -o fuseaw $(pkg-config fuse3 --cflags --libs)
```

## Example

```shell
$ ./fuseaw '/mnt/gamedrv/genshin_impact/Genshin Impact/GenshinImpact_Data/StreamingAssets/AudioAssets/MusicGame/MusicGame11.pck' /tmp/fuseaw
$ tree /tmp/fuseaw
/tmp/fuseaw
├── sounds
│   └── sfx
│       ├── 0x113eebf9.wem
│       ├── 0x216efe11.wem
│       ├── 0x29bedc1b.wem
│       ├── 0x2f82cf74.wem
│       ├── 0x32087a15.wem
│       ├── 0x32b32c09.wem
│       ├── 0x347dee95.wem
│       ├── 0x35a46625.wem
│       ├── 0x3701b3fb.wem
│       ├── 0x3c845b18.wem
│       ├── 0x3f4be383.wem
│       └── 0xb8f08ff.wem
└── source.pck -> /mnt/gamedrv/genshin_impact/Genshin Impact/GenshinImpact_Data/StreamingAssets/AudioAssets/MusicGame/MusicGame11.pck

3 directories, 13 files
```

## Goals and no-goals

**Goals, accomplished:**

* Mount `.pck` files as a directiry
* Extract audio files fro the `.pck`, as well as unpacking `.bnk` files for embedded `.wem` files

**Goals, to be done:**

* File name lookup using a mapping
* Endfield `.chk` file support

**No-goals:**

* Inspection of `.hdiff` files
  * If you want to look into `.hdiff` files, create a merged copy and run `fuseaw` on it.

## Compactibility

**Tested and works on:**

* Genshin Impact
* Honkai: Star Rail

**Not tested, expect limited compactibility:**

* Zenless Zone Zero

**Imcompactible yet:**

* Arknights: Endfield
