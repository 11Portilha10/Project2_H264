#ifndef NAL_UNIT_H_
#define NAL_UNIT_H_

#include <string>
#include <cstdint>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cmath>

#include "vlc.h"
#include "tr_qt.h"
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

/**
 * Coded or transmitted H.264 data is stored or transmitted as a series of packets
 * known as Network Abstraction Layer Units (NAL Units).
 * 
 * | 1 byte NALU header     | byte stream |
 * | NALU Type | Importance | byte stream |
 * 
 */
class NALUnit {
public:
  Bitstream buffer;   // the byte stream of the NAL Unit

  NALUnit(const NALRefIdc, const NALType, const Bitstream&);
  std::uint8_t nal_header();
  Bitstream get();

private:
  int forbidden_zero_bit; // Always be zero
  NALRefIdc nal_ref_idc;  // Importance for the decoder
  NALType nal_unit_type;  // NAL Unit type
};

#endif