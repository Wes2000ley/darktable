/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.
    Portions adapted from the Nikon HE decoder, Copyright (C) 2026 Dmitri Sotnikov.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable. If not, see <http://www.gnu.org/licenses/>.
*/

#include "imageio/imageio_nikon_he.h"
#include "imageio/nikon_he/nikon_he_decode.h"
#include "imageio/nikon_he/nikon_he_gtli_table.h"
#include "imageio/nikon_he/nikon_he_picture_header.h"

#include "adt/Array2DRef.h"
#include "common/RawImage.h"
#include "decoders/RawDecoder.h"
#include "decoders/RawDecoderException.h"
#include "io/Buffer.h"
#include "tiff/TiffIFD.h"
#include "tiff/TiffTag.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
class _picture_header_scope
{
public:
  explicit _picture_header_scope(const nikon_he::PictureHeader &header)
  {
    nikon_he::set_active_picture_header(&header);
  }
  ~_picture_header_scope()
  {
    nikon_he::set_active_picture_header(nullptr);
  }
  _picture_header_scope(const _picture_header_scope &) = delete;
  _picture_header_scope &operator=(const _picture_header_scope &) = delete;
};
}

bool dt_imageio_decode_nikon_he(rawspeed::RawDecoder &decoder, const rawspeed::Buffer file)
{
  using namespace rawspeed;

  const TiffIFD *root = decoder.getRootIFD();
  if(!root) return false;
  const auto *make = root->getEntryRecursive(TiffTag::MAKE);
  const auto *model = root->getEntryRecursive(TiffTag::MODEL);
  if(!make || !model || make->getString() != "NIKON CORPORATION"
     || model->getString() != "NIKON Z5_2")
    return false;

  const auto *compression = root->getEntryRecursive(static_cast<TiffTag>(0x0051));
  if(!compression) return false;
  // the compression value is a short at byte 10 of Nikon's RawImageInfo block
  auto raw_info = compression->getData();
  raw_info.skipBytes(10);
  const uint16_t nef_compression = raw_info.getU16();
  if(nef_compression != 13 && nef_compression != 14) return false;

  const TiffIFD *raw = root->getIFDWithTag(TiffTag::CFAPATTERN);
  if(raw->getEntry(TiffTag::COMPRESSION)->getU32() != 34713
     || raw->getEntry(TiffTag::BITSPERSAMPLE)->getU32() != 14)
    ThrowRDE("invalid Z5II HE compression or bit depth");

  const uint32_t width = raw->getEntry(TiffTag::IMAGEWIDTH)->getU32();
  const uint32_t height = raw->getEntry(TiffTag::IMAGELENGTH)->getU32();
  // the codec uses whole four-coefficient groups and pairs of Bayer rows
  if(width < 64 || width > 6064 || width % 8 || height < 64 || height > 4040 || height % 2)
    ThrowRDE("unsupported Z5II HE dimensions: %u x %u", width, height);

  const auto *offsets = raw->getEntry(TiffTag::STRIPOFFSETS);
  const auto *counts = raw->getEntry(TiffTag::STRIPBYTECOUNTS);
  if(offsets->count != 1 || counts->count != 1)
    ThrowRDE("invalid Z5II HE strip count");
  const Buffer strip = file.getSubView(offsets->getU32(), counts->getU32());

  nikon_he::PictureHeader header;
  if(!nikon_he::parse_picture_header(strip.begin(), strip.getSize(), header)
     || !nikon_he::is_supported_picture_header(header, strip.getSize())
     || !header.has_vendor_tail || header.precinct_offset >= strip.getSize())
    ThrowRDE("unsupported Nikon HE picture header");
  if(header.hdr_width != width || header.hdr_height != height)
    ThrowRDE("picture dimensions disagree with the NEF container");

  // the picture header contains the exact transfer curve for this image
  const uint8_t *tail = header.vendor_tail;
  const int64_t b = (static_cast<int64_t>(tail[3] & 15) << 8) | tail[4];
  const int64_t a = (static_cast<int64_t>(tail[7] & 3) << 16) | (tail[8] << 8) | tail[9];
  const int64_t c = (tail[10] << 8) | tail[11];
  std::vector<int32_t> transfer(nikon_he::kLutSize);
  for(int i = 0; i < nikon_he::kLutSize; i++)
  {
    const int64_t delta = std::min(i, 65535) - c;
    const int64_t scaled = (delta * a + 32768) >> 16;
    const int64_t squared = delta < 0 ? -delta * delta : scaled * scaled;
    const int64_t value = std::clamp(b + ((squared + 163840) >> 18), int64_t(0), int64_t(16383));
    transfer[i] = static_cast<int32_t>(4 * value);
  }

  std::vector<uint16_t> bayer(static_cast<size_t>(width) * height);
  const _picture_header_scope active_header(header);
  const auto result = nikon_he::decode_nikon_he_image(
    strip.begin() + header.precinct_offset, strip.getSize() - header.precinct_offset,
    width, height, transfer.data(), bayer.data());
  if(!result.success)
    ThrowRDE("corrupt Nikon HE precinct stream");

  const RawImage image = decoder.mRaw;
  image->dim = iPoint2D(width, height);
  image->createData();
  const auto output = image->getU16DataAsUncroppedArray2DRef();
  for(uint32_t row = 0; row < height; row++)
    std::memcpy(&output(row, 0), bayer.data() + static_cast<size_t>(row) * width,
                width * sizeof(uint16_t));
  return true;
}
// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
