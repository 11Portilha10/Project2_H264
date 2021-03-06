#ifndef PACKAGER_H_
#define PACKAGER_H_

#include <string>
#include <cstdint>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cmath>

#include "vlc.h"
#include "nal_unit.h"
#include "tr_qt.h"
#include "frame.h"
#include "bitstream.h"

class Packager {
public:
  Packager(std::string);

  void write_SPS(const int, const int, const int);
  void write_PPS();
  void write_slice(const int, Frame&);

private:
  std::fstream file;
  static std::uint8_t start_code[4];
  unsigned int log2_max_frame_num;
  unsigned int log2_max_pic_order_cnt_lsb;

  Bitstream seq_parameter_set_rbsp(const int, const int, const int);
  Bitstream pic_parameter_set_rbsp();
  Bitstream write_slice_data(Frame&, Bitstream&);
  Bitstream mb_pred(MacroBlock&, Frame&);
  Bitstream slice_layer_without_partitioning_rbsp(const int, Frame&);
  Bitstream slice_header(const int);
};

#endif