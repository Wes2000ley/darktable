add_library(darktable_nikon_he STATIC
  ../imageio_nikon_he.cc
  nikon_he_bayer.cpp
  nikon_he_bit_reader.cpp
  nikon_he_coefficient_decode.cpp
  nikon_he_decode.cpp
  nikon_he_dequantize.cpp
  nikon_he_gcli_decode.cpp
  nikon_he_gtli_table.cpp
  nikon_he_idwt_horizontal.cpp
  nikon_he_idwt_vertical.cpp
  nikon_he_picture_header.cpp
  nikon_he_precinct_decode.cpp
  nikon_he_precinct_header.cpp
  nikon_he_predecessor.cpp
  nikon_he_predict_lut.cpp
  nikon_he_subband_config.cpp
  nikon_he_tile.cpp)

target_link_libraries(darktable_nikon_he PUBLIC rawspeed)
set_target_properties(darktable_nikon_he PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(FILES README.md LICENSE.LGPL LICENSE.CDDL
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/nikon_he COMPONENT DTDocuments)
