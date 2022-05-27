#include "nal_unit.h"

/* Construct NAL unit 
 * given ref, type and RBSP
 */
NALUnit::NALUnit(const NALRefIdc ref_idc, const NALType type, const Bitstream& rbsp) {
  forbidden_zero_bit = 0;
  nal_ref_idc = ref_idc;
  nal_unit_type = type;
  buffer = rbsp;
}


/**
 * @brief Return NAL header, which has 8 bits 
 *          1                2             5
 * |------------------|------------|--------------|
 * 
 *  forbidden_zero_bit  nal_ref_idc  nal_unit_type 
 */
std::uint8_t NALUnit::nal_header() {
  return (forbidden_zero_bit << 7) | (static_cast<uint8_t>(nal_ref_idc) << 5) |
           (static_cast<uint8_t>(nal_unit_type));
}

/**
 * @brief Return NAL Unit bitstream, including header
 * 
 */
Bitstream NALUnit::get() {
  std::uint8_t header[] = {nal_header()};
  Bitstream output(header, 8);  // create header bitstream (1 byte)
  return output + buffer;     // return header + content stream
}
