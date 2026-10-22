/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

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

#include <glib.h>
#include <libraw/libraw.h>

#include <cstdio>
#include <cstring>
#include <memory>

#ifdef _WIN32
#include "win/main_wrapper.h"
#endif

// CC0 samples and golden mosaics from the pinned LibRaw's redlamp/bench directory
struct sample_t
{
  const char *name;
  unsigned width, height, compression;
  const char *sha256;
};

static const sample_t samples[] = {
  { "Z5_2_16x9_HE_6d087e88", 6064, 3416, 13,
    "703c7c15f366a52b419319cf7ee1744aacbc6484ba8a4ecaae041b748e333994" },
  { "Z5_2_16x9_HEstar_06069e1c", 6064, 3416, 14,
    "a16e52433efac5c942a09e7d098c099f84a6087197448531d30aba28ea75dbdb" },
  { "Z5_2_1x1_HE_465aba7a", 4048, 4040, 13,
    "1d642854161456fe4c927328bfa60d378403b7ae97282d0a18538be37464c844" },
  { "Z5_2_1x1_HEstar_a605497e", 4048, 4040, 14,
    "9014140fd54c00c7fa2a5ef8c6309d16694995849f1b11c298da2adb87d5a1ad" },
  { "Z5_2_3x2_HE_196971ea", 4000, 2672, 13,
    "090d6e66ca9b65997120b129492f07fd977b2948e6a8d501c79e89c1dd43c0a2" },
  { "Z5_2_3x2_HE_d8938c5d", 6064, 4040, 13,
    "cc8439d8ac026c9f4e085a40e1b364bb82690bd91ed4d4ec924bb3979073a6e5" },
  { "Z5_2_3x2_HEstar_276ef4ad", 4000, 2672, 14,
    "d59e7ea4250ff32d1152265126e5ed22987d3e71e3b2d4f1ceca87dada5fee0b" },
  { "Z5_2_3x2_HEstar_b44bb317", 6064, 4040, 14,
    "2e0248af6a801d2586d0089b69836dd00b8059dae01d75c41ae5e4e37f4a6804" }
};

static bool _check_sample(const char *directory, const sample_t &sample)
{
  gchar *path = g_strdup_printf("%s/%s.NEF", directory, sample.name);
  gchar *contents = nullptr;
  gsize size = 0;
  const gboolean read = g_file_get_contents(path, &contents, &size, nullptr);
  g_free(path);
  if(!read)
  {
    std::fprintf(stderr, "missing sample: %s\n", sample.name);
    return false;
  }

  // LibRaw exceeds the default stack size of some Windows threads
  auto raw = std::make_unique<LibRaw>();
  int error = raw->open_buffer(contents, size);
  if(error == LIBRAW_SUCCESS) error = raw->unpack();
  bool valid = error == LIBRAW_SUCCESS;
  if(valid)
  {
    const auto &data = raw->imgdata;
    const auto &sizes = data.sizes;
    valid = data.rawdata.raw_image && sizes.raw_width == sample.width
      && sizes.raw_height == sample.height && data.color.black == 1008
      && data.color.maximum == 16383
      && data.makernotes.nikon.NEFCompression == sample.compression
      && !std::strcmp(data.idata.model, "Z5_2");
    if(valid)
    {
      GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
      for(unsigned row = 0; row < sizes.raw_height; row++)
        g_checksum_update(checksum,
                          reinterpret_cast<const guchar *>(data.rawdata.raw_image)
                            + row * sizes.raw_pitch,
                          sizes.raw_width * sizeof(unsigned short));
      valid = !std::strcmp(g_checksum_get_string(checksum), sample.sha256);
      g_checksum_free(checksum);
    }
  }
  raw->recycle();

  // a truncated precinct stream must fail rather than produce a partial mosaic
  error = raw->open_buffer(contents, size / 2);
  if(error == LIBRAW_SUCCESS) error = raw->unpack();
  valid = valid && error != LIBRAW_SUCCESS;
  raw.reset();
  g_free(contents);
  std::printf("%s: %s\n", sample.name, valid ? "PASS" : "FAIL");
  return valid;
}

int main(int argc, char *argv[])
{
  const char *directory = g_getenv("NIKON_Z5II_SAMPLE_DIR");
  if(!directory || !*directory)
  {
    std::puts("set NIKON_Z5II_SAMPLE_DIR to the verified CC0 sample directory");
    return 77;
  }

  bool valid = true;
  for(const sample_t &sample : samples)
    valid = _check_sample(directory, sample) && valid;
  return valid ? 0 : 1;
}
// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
