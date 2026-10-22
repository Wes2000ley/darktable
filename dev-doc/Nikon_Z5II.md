# Nikon Z5II High Efficiency NEFs

Darktable decodes the Z5II's 14-bit High Efficiency and High Efficiency*
NEFs with an in-tree codec in `src/imageio/nikon_he`. It adds no external
dependencies and leaves every submodule URL and revision unchanged.
The codec works with `USE_LIBRAW=ON` or `USE_LIBRAW=OFF`, including builds
that use a system LibRaw.

## Decoding and metadata

RawSpeed parses the NEF container and checks the camera's support against
`cameras.xml`. Before its normal decompression step,
`imageio_nikon_he.cc` checks the make/model and the Nikon RawImageInfo
maker-note block (tag `0x0051`). Its short at byte offset 10 identifies HE
as `13` and HE* as `14` (`0x0e`). This is separate from TIFF Compression,
which remains `34713`.

Only Z5II HE/HE* images use the native codec. Other formats, including
lossless NEFs, use their existing decoding paths. The adapter validates
the 14-bit format, dimensions, strip range and codestream profile before
decompression. The picture header supplies the exact transfer curve.
The codec returns the full 14-bit RGGB Bayer mosaic without demosaicing.

RawSpeed then reads metadata normally, including white balance, and applies
the existing Z5II crop, black level, white level and color matrix from
`data/cameras.xml`. No separate camera matrix is hardcoded in the loader.
Nikon's existing `libraw_extensions` override behavior is unchanged.

The codec was adapted from the community HE decoder and fixes at
[Redlamp LibRaw 6c5f4637a73e69074119ed0c38662027bca84982](https://github.com/pdcgomes/redlamp-libraw/commit/6c5f4637a73e69074119ed0c38662027bca84982).
See `src/imageio/nikon_he/README.md` for attribution, licenses and the
changes made during integration.

## Regression checks

Configure with `-DBUILD_TESTING=ON`. Download the eleven CC0 raw.pixls.us
samples listed in `src/tests/nikon_z5ii_samples.json`, verify their SHA-256
hashes, and save them as `<name>.NEF`. Set `NIKON_Z5II_SAMPLE_DIR` to that
directory before running:

```sh
cmake --build build --target darktable-test-nikon-z5ii
ctest --test-dir build -R '^imageio_nikon_z5ii$' --output-on-failure
```

The test checks every photosite of eight HE/HE* files against the source
decoder's golden hashes, plus dimensions, compression values, calibration
and white balance availability. Hashes use unsigned 16-bit little-endian
photosites without row padding. They establish regression consistency,
not independent accuracy against Nikon's decoder.

It also rejects truncated input, damaged picture headers, empty precinct
streams, impossible packet lengths and inconsistent dimensions. Three
lossless NEFs exercise the unmodified RawSpeed fallback. Concurrent decodes
with different dimensions and HE profiles check for shared mutable state.
Without the sample directory CTest reports a skipped test. Additional local
HE/HE* files can be checked by passing their paths to the test executable.

Repeat the build and test with `-DUSE_LIBRAW=OFF` to verify independence from
LibRaw. Exercise the application with isolated configuration and cache
directories and an uncompressed PPM export:

```sh
darktable-cli sample.NEF sample.ppm --width 1280 --height 1280 \
  --apply-custom-presets false --core --configdir ./test-config \
  --cachedir ./test-cache --disable-opencl
```

Include HE, HE*, full-frame, DX, square and 16:9 samples, and lossless NEFs.
Compare lossless exports against a build from the unchanged dependency
baseline. The CLI uses an in-memory library and disables sidecar writing.
