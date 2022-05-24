#ifndef NAL
#define NAL

#include <string>
#include <cstdint>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cmath>

#include "vlc.h"
#include "nal.h"
#include "qdct.h"
#include "frame.h"
#include "bitstream.h"

/* For nal_unit_type
 * 2, 3, 4 do not appear in baseline profile
 */
enum class NALType {
 SLICE    = 1,
 DPA      = 2,
 DPB      = 3,
 DPC      = 4,
 IDR      = 5,
 SEI      = 6,
 SPS      = 7,
 PPS      = 8,
 AUD      = 9,
 EOSEQ    = 10,
 EOSTREAM = 11,
 FILL     = 12
};

/* For nal_ref_idc
 * the priority of NAL unit
 */
enum class NALRefIdc {
 HIGHEST     = 3,
 HIGH        = 2,
 LOW         = 1,
 DISPOSABLE  = 0
};

class NALUnit {
public:
  Bitstream buffer;

  NALUnit(const NALRefIdc, const NALType, const Bitstream&);
  std::uint8_t nal_header();
  Bitstream get();

private:
  int forbidden_zero_bit; // Always be zero
  NALRefIdc nal_ref_idc;
  NALType nal_unit_type;
};

class Writer {
public:
  Writer(std::string);

  void write_sps(const int, const int, const int);
  void write_pps();
  void write_slice(const int, Frame&);

private:
  // Log logger;
  std::fstream file;
  static std::uint8_t stopcode[4];
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