# Nikon HE codec

This directory contains the Nikon High Efficiency / High Efficiency* codec
compiled directly into darktable. It uses the C++ standard library and
darktable's existing RawSpeed dependency. It does not link to LibRaw or
require another package.

The codec sources were imported from
[pdcgomes/redlamp-libraw at 6c5f4637a73e69074119ed0c38662027bca84982](https://github.com/pdcgomes/redlamp-libraw/tree/6c5f4637a73e69074119ed0c38662027bca84982/src/decoders/nikon_he).
That version incorporates the decoder by Dmitri Sotnikov, Yogthos and
Jiang Yaogeng from [LibRaw PR 826](https://github.com/LibRaw/LibRaw/pull/826),
RdWing's decoding fixes, and Redlamp's fixes for stream layout, input
validation and concurrent decoding. Original copyright and license notices
are preserved. Darktable uses the LGPL-2.1 option; copies of both licenses
offered by the original sources are included here.

Changes to the imported codec:

- Release per-tile prediction storage on normal and exceptional exits.
- Use scoped containers for image and precinct scratch allocations.
- Reject entropy substreams that run out of bits instead of padding them
  into a partial image.
- Remove unused local variables rejected by darktable's compiler warnings.
- Omit the unused approximate transfer-curve table. The integration uses
  the exact transfer parameters from each picture header.

`../imageio_nikon_he.cc` supplies the darktable integration and exact
transfer curve, adapted from the same revision's `nikon_he_decoder.cpp`.
It identifies Z5II HE/HE* files through RawSpeed's parsed TIFF tree and
fills its RAW image. RawSpeed then reads white balance and applies the
camera database's crop, sensor levels and color matrix normally. Neither
RawSpeed nor LibRaw source files or dependency revisions are modified.

See `dev-doc/Nikon_Z5II.md` for build and regression checks. The regression
hashes establish consistency with the source decoder; they are not an
independent verification against Nikon's decoder.
