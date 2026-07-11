# AnimeWwise FUSE (`fuseaw`)

[FUSE](https://github.com/libfuse/libfuse) binding for extracting audio from some anime games.

Concepts taken and code derived from [AnimeWwise](https://github.com/Escartem/AnimeWwise).

## Compile

```bash
g++ -std=c++23 fuseaw.cpp src/*.cpp -o fuseaw $(pkg-config fuse3 --cflags --libs)
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
