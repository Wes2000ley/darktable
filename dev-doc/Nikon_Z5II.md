# Nikon Z5II High Efficiency NEFs

This fork decodes the Z5II's 14-bit High Efficiency and High Efficiency*
NEFs with the bundled LibRaw. Nikon maker-note tag `0x0051` identifies HE
as `13` and HE* as `14` (`0x0e`). This is separate from the TIFF Compression
tag, which remains `34713`. File size varies with scene content and crop.

The bundled LibRaw is pinned to
[`6c5f4637a73e69074119ed0c38662027bca84982`](https://github.com/pdcgomes/redlamp-libraw/commit/6c5f4637a73e69074119ed0c38662027bca84982),
which carries LibRaw PR 826's decoder and Redlamp's fixes for stream layout,
transfer curves, malformed input and concurrent decoding. Its source files
retain their authorship and LibRaw's LGPL-2.1/CDDL-1.0 license choice.
See its `REDLAMP.md` for provenance and changes from upstream.

NEFs are identified from their container and opened with LibRaw before
RawSpeed. Automatic LibRaw decoding is restricted to the Z5II's HE/HE*
maker-note values. Lossless NEFs continue through RawSpeed. Explicit
`libraw_extensions` overrides retain their existing behavior.

The HE path uses the Z5II matrix and sensor white level already present in
RawSpeed's `data/cameras.xml`, as this LibRaw revision lacks the model's
color matrix. White balance, black levels, Bayer samples and crop geometry
come from LibRaw.

## Build

Initialize submodules after cloning this fork, then follow the normal build
instructions. Keep `USE_LIBRAW=ON` and `DONT_USE_INTERNAL_LIBRAW=OFF` (the
defaults). A system LibRaw build only has HE support if that library provides
the decoder. Builds with `USE_LIBRAW=OFF` retain ordinary NEF support.

## Regression checks

Configure with `-DBUILD_TESTING=ON`. Download the eight Z5II HE/HE* samples
listed in `src/external/LibRaw/redlamp/bench/samples.json`, verify their
SHA-256 hashes, and save them as `<name>.NEF`. Set `NIKON_Z5II_SAMPLE_DIR`
to that directory before running:

```sh
cmake --build build --target darktable-test-nikon-z5ii
ctest --test-dir build -R '^imageio_nikon_z5ii$' --output-on-failure
```

The test checks every photosite against the pinned decoder's recorded
golden hashes, dimensions, black/white levels and compression values. It
also checks that truncated streams fail. Without the sample directory,
CTest reports a skipped test.

Exercise the application path with isolated configuration and cache
directories and an uncompressed PPM export:

```sh
darktable-cli sample.NEF sample.ppm --width 1280 --height 1280 \
  --apply-custom-presets false --core --configdir ./test-config \
  --cachedir ./test-cache --disable-opencl
```

Include HE, HE*, full-frame, DX, square and 16:9 samples, and lossless NEFs
to verify routing. Repeat a lossless export with `USE_LIBRAW=OFF` and
compare the output bytes. The CLI uses an in-memory library and disables
sidecar writing.
