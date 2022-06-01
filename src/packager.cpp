#include "packager.h"

// Start/stop code prefix to separate NAL Units
std::uint8_t Packager::start_code[4] = {0x00, 0x00, 0x00, 0x01};

Packager::Packager(std::string filename) {
  // Open the file stream for output file
  file.open(filename, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    exit(1);
  }
}

/**
 * @brief Writes the Sequence Parameter Set with data from the sequence of frames
 * 
 * @param width   Width of the video frames
 * @param height  Height of the video frames
 * @param num_frames  Number of frames in the stream (PC Range images in this case)
 */
void Packager::write_SPS(const int width, const int height, const int num_frames) {
  Bitstream output(start_code, 32);
  Bitstream rbsp = seq_parameter_set_rbsp(width, height, num_frames);   // SPS raw byte sequence payload
  NALUnit nal_unit(NALRefIdc::HIGHEST, NALType::SPS, rbsp.rbsp_to_ebsp());  // construct SPS NAL Unit

  output += nal_unit.get();

  file.write((char*)&output.buffer[0], output.buffer.size());
  file.flush();
}

void Packager::write_PPS() {
  Bitstream output(start_code, 32);
  Bitstream rbsp = pic_parameter_set_rbsp();
  NALUnit nal_unit(NALRefIdc::HIGHEST, NALType::PPS, rbsp.rbsp_to_ebsp());

  output += nal_unit.get();

  file.write((char*)&output.buffer[0], output.buffer.size());
  file.flush();
}

/**
 * @brief Writes slice header and data (1 frame per slice)
 * 
 * @param frame_num Frame number (starting from zero)
 * @param frame The Frame instance (Range image)
 */
void Packager::write_slice(const int frame_num, Frame& frame) {
  Bitstream output(start_code, 32);
  Bitstream rbsp = slice_layer_without_partitioning_rbsp(frame_num, frame);
  rbsp += Bitstream((std::uint8_t)0x80, 8);

  NALUnit nal_unit(NALRefIdc::HIGHEST, NALType::IDR, rbsp.rbsp_to_ebsp());

  output += nal_unit.get();
  file.write((char*)&output.buffer[0], output.buffer.size());
  file.flush();
}

/**
 * @brief Generates the SPS Raw Byte Sequence Payload
 * 
 * @param width   Width of frames in the stream (Range images)
 * @param height  Height of frames in the stream (Range images)
 * @param num_frames  Number of frames (Range images) in the stream
 * 
 * @return RBSP of the SPS (without NALU header)
 * 
 * @note Baseline profile only!
 */
Bitstream Packager::seq_parameter_set_rbsp(const int width, const int height, const int num_frames) {
  Bitstream sodb;
  std::uint8_t profile_idc = 66;  // u(8)   // baseline profile
  bool constraint_set0_flag = false;  // u(1)
  bool constraint_set1_flag = false;  // u(1)
  bool constraint_set2_flag = false;  // u(1)
  bool constraint_set3_flag = false;  // u(1)   <------ ADDED
  // std::uint8_t reserved_zero_5bits = 0x00;  // u(5)
  std::uint8_t reserved_zero_4bits = 0x00;  // u(4)   <------ MODIFIED
  std::uint8_t level_idc = 10;  // u(8)
  unsigned int seq_parameter_set_id = 0;  // ue(v)
  unsigned int log2_max_frame_num_minus4 = std::max(0, (int)log2(num_frames) - 4); // ue(v)
  unsigned int pic_order_cnt_type = 0;  // ue(v)
  unsigned int log2_max_pic_order_cnt_lsb_minus4 = log2_max_frame_num_minus4; // ue(v)
  unsigned int num_ref_frames = 0;  // ue(v)
  bool gaps_in_frame_num_value_allowed_flag = false;  // u(1)
  unsigned int pic_width_in_mbs_minus_1 = (width % 16 == 0)? (width / 16) - 1 : width / 16; // ue(v)
  unsigned int pic_height_in_mbs_minus_1 = (height % 16 == 0)? (height / 16) - 1 : height / 16; // ue(v)
  bool frame_mbs_only_flag = true;  // u(1)
  bool direct_8x8_inference_flag = false; // u(1)
  bool frame_cropping_flag = (width % 16 != 0) || (height % 16 != 0); // u(1)

  // if (frame_cropping_flag)
  unsigned int frame_crop_left_offset = 0;  // ue(v)
  unsigned int frame_crop_right_offset = ((pic_width_in_mbs_minus_1 + 1) * 16 - width) / 2; // ue(v)
  unsigned int frame_crop_top_offset = 0; // ue(v)
  unsigned int frame_crop_bottom_offset = ((pic_height_in_mbs_minus_1 + 1) * 16 - height) / 2;  // ue(v)

  bool vui_parameters_present_flag = false; // u(1)

  // slice header info
  log2_max_frame_num = log2_max_frame_num_minus4 + 4;
  log2_max_pic_order_cnt_lsb = log2_max_pic_order_cnt_lsb_minus4 + 4;

  sodb += Bitstream(profile_idc, 8);
  sodb += Bitstream(constraint_set0_flag);
  sodb += Bitstream(constraint_set1_flag); 
  sodb += Bitstream(constraint_set2_flag);
  sodb += Bitstream(constraint_set3_flag);    // ADDED
  // sodb += Bitstream(reserved_zero_5bits, 5);
  sodb += Bitstream(reserved_zero_4bits, 4);  // MODIFIED
  sodb += Bitstream(level_idc, 8);
  sodb += uegc(seq_parameter_set_id);
  sodb += uegc(log2_max_frame_num_minus4);
  sodb += uegc(pic_order_cnt_type); 
  sodb += uegc(log2_max_pic_order_cnt_lsb_minus4);
  sodb += uegc(num_ref_frames); 
  sodb += Bitstream(gaps_in_frame_num_value_allowed_flag);
  sodb += uegc(pic_width_in_mbs_minus_1); 
  sodb += uegc(pic_height_in_mbs_minus_1);
  sodb += Bitstream(frame_mbs_only_flag); 
  sodb += Bitstream(direct_8x8_inference_flag);
  sodb += Bitstream(frame_cropping_flag);

  if (frame_cropping_flag) {
    sodb += uegc(frame_crop_left_offset);
    sodb += uegc(frame_crop_right_offset);
    sodb += uegc(frame_crop_top_offset);
    sodb += uegc(frame_crop_bottom_offset);
  }

  sodb += Bitstream(vui_parameters_present_flag);

  return sodb.rbsp_trailing_bits();
}

/**
 * @brief Generates the SPS Raw Byte Sequence Payload
 * 
 * @return Bitstream 
 */
Bitstream Packager::pic_parameter_set_rbsp() {
  const int QPC2idoffset[] = {
    0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 30,
    31, 32, 33, 35, 36, 38, 40, 42, 45, 48
  };

  Bitstream sodb;

  unsigned int pic_parameter_set_id = 0;  // ue(v)
  unsigned int seq_parameter_set_id = 0;  // ue(v)
  bool entropy_coding_mode_flag = false;  // u(1)
  bool pic_order_present_flag = false;  // u(1)
  unsigned int num_slice_groups_minus1 = 0; // ue(v)
  unsigned int num_ref_idx_l0_active_minus1 = 0;  // ue(v)
  unsigned int num_ref_idx_l1_active_minus1 = 0;  // ue(v)
  bool weighted_pred_flag = false;  // u(1)
  unsigned int weighted_bipred_idc = 0; // u(2)
  int pic_init_qp_minus26 = LUMA_QP - 26; // se(v)
  int pic_init_qs_minus26 = 0;  // se(v)
  int chroma_qp_index_offset = QPC2idoffset[CHROMA_QP] - LUMA_QP; // se(v)
  bool deblocking_filter_control_present_flag = true; // u(1)
  bool constrained_intra_pred_flag = false; // u(1)
  bool redundant_pic_cnt_present_flag = false;  // u(1)

  sodb += uegc(pic_parameter_set_id); 
  sodb += uegc(seq_parameter_set_id);
  sodb += Bitstream(entropy_coding_mode_flag); 
  sodb += Bitstream(pic_order_present_flag);
  sodb += uegc(num_slice_groups_minus1); 
  sodb += uegc(num_ref_idx_l0_active_minus1);
  sodb += uegc(num_ref_idx_l1_active_minus1); 
  sodb += Bitstream(weighted_pred_flag);
  sodb += Bitstream(weighted_bipred_idc, 2); 
  sodb += segc(pic_init_qp_minus26);
  sodb += segc(pic_init_qs_minus26); 
  sodb += segc(chroma_qp_index_offset);
  sodb += Bitstream(deblocking_filter_control_present_flag); 
  sodb += Bitstream(constrained_intra_pred_flag);
  sodb += Bitstream(redundant_pic_cnt_present_flag);

  return sodb.rbsp_trailing_bits();
}

Bitstream Packager::slice_layer_without_partitioning_rbsp(const int _frame_num, Frame& frame) {
  Bitstream sodb = slice_header(_frame_num);    // write slice header
  return write_slice_data(frame, sodb).rbsp_trailing_bits();
}

Bitstream Packager::write_slice_data(Frame& frame, Bitstream& sodb) {
  for (auto& mb : frame.mbs) {
    if (mb.is_I_PCM) {    // MB not intra coded
      sodb += uegc(25);

      while(!sodb.byte_align())
        sodb += Bitstream(false);

      for (auto& y : mb.Y)
        sodb += Bitstream(static_cast<std::uint8_t>(y), 8);

      for (auto& cb : mb.Cb)
        sodb += Bitstream(static_cast<std::uint8_t>(cb), 8);

      for (auto& cr : mb.Cr)
        sodb += Bitstream(static_cast<std::uint8_t>(cr), 8);

      continue;
    }

    // Encode mb_type and pred mode for 16x16 Luma
    if (mb.is_intra16x16) {
      unsigned int type = 1;
      if (mb.coded_block_pattern_luma)
        type += 12;

      if (mb.coded_block_pattern_chroma_DC == false && mb.coded_block_pattern_chroma_AC == false)
        type += 0;
      else if (mb.coded_block_pattern_chroma_AC == false)
        type += 4;
      else
        type += 8;

      type += static_cast<unsigned int>(mb.intra16x16_Y_mode);
      sodb += uegc(type);
    } else {
      sodb += uegc(0);
    }

    sodb += mb_pred(mb, frame);   // encode intra-prediction modes for 4x4 Luma and Chroma

    // Encode coded_block_pattern syntax element for MBs other than 16x16 Luma
    if (!mb.is_intra16x16) {
      unsigned int cbp = 0;
      if (mb.coded_block_pattern_chroma_DC == false && mb.coded_block_pattern_chroma_AC == false)
        cbp += 0;
      else if (mb.coded_block_pattern_chroma_AC == false)
        cbp += 16;
      else
        cbp += 32;

      for (int i = 0; i != 4; i++)
        if (mb.coded_block_pattern_luma_4x4[i])
          cbp += (1 << i);

      sodb += uegc(me[cbp]);
    }

    // Add residual data
    if (mb.coded_block_pattern_luma || mb.coded_block_pattern_chroma_DC || mb.coded_block_pattern_chroma_AC || mb.is_intra16x16) {
      sodb += segc(0);  // delta_qp
      sodb += mb.bitstream;
    }
  }

  return sodb;
}

/**
 * @brief Encode intra-prediction modes for each MB
 * 
 * @param mb The current MB
 * @param frame The current frame (range image)
 * @return Bitstream with the encoded pred modes
 */
Bitstream Packager::mb_pred(MacroBlock& mb, Frame& frame) {
  Bitstream sodb;

  if (!mb.is_intra16x16) {
    for (int cur_pos = 0; cur_pos != 16; cur_pos++) {
      int real_pos = MacroBlock::convert_table[cur_pos];

      int pmA_index, pmA_pos;
      if (real_pos % 4 == 0) {
        pmA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
        pmA_pos = real_pos + 3;
      } else {
        pmA_index = mb.mb_index;
        pmA_pos = real_pos - 1;
      }
      pmA_pos = MacroBlock::convert_table[pmA_pos];

      int pmB_index, pmB_pos;
      if (0 <= real_pos && real_pos <= 3) {
        pmB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
        pmB_pos = 12 + real_pos;
      } else {
        pmB_index = mb.mb_index;
        pmB_pos = real_pos - 4;
      }
      pmB_pos = MacroBlock::convert_table[pmB_pos];

      int pred_modeA = 2, pred_modeB = 2;
      if (pmA_index != -1 && (!frame.mbs.at(pmA_index).is_intra16x16) && (!frame.mbs.at(pmA_index).is_I_PCM)
          && pmB_index != -1 && (!frame.mbs.at(pmB_index).is_intra16x16) && (!frame.mbs.at(pmB_index).is_I_PCM)) {
        pred_modeA = static_cast<int>(frame.mbs.at(pmA_index).intra4x4_Y_mode.at(pmA_pos));
        pred_modeB = static_cast<int>(frame.mbs.at(pmB_index).intra4x4_Y_mode.at(pmB_pos));
      }

      int pred_mode = std::min(pred_modeA, pred_modeB);
      int cur_mode = static_cast<int>(mb.intra4x4_Y_mode.at(cur_pos));
      if (pred_mode == cur_mode) {
        sodb += Bitstream(true);
      } else {
        sodb += Bitstream(false);
        if (cur_mode < pred_mode)
          sodb += Bitstream(static_cast<std::uint8_t>(cur_mode), 3);
        else
          sodb += Bitstream(static_cast<std::uint8_t>(cur_mode - 1), 3);
      }
    }
  }

  sodb += uegc(static_cast<unsigned int>(mb.intra_Cr_Cb_mode));

  return sodb;
}

Bitstream Packager::slice_header(const int _frame_num) {
  Bitstream sodb;

  unsigned int first_mb_in_slice = 0;  // ue(v)
  unsigned int slice_type = 2; // ue(v)
  unsigned int pic_parameter_set_id = 0; // ue(v)
  unsigned int frame_num = 0;  // u(v)
  unsigned int idr_pic_id = _frame_num; // ue(v)
  unsigned int pic_order_cnt_lsb = _frame_num;  // u(v)
  bool no_output_of_prior_pics_flag = true; // u(1)
  bool long_term_reference_flag = false; // u(1)
  int slice_qp_delta = 0;  // se(v)
  unsigned int disable_deblocking_filter_idc = 1; // ue(v)

  sodb += uegc(first_mb_in_slice); 
  sodb += uegc(slice_type);
  sodb += uegc(pic_parameter_set_id); 
  sodb += Bitstream(frame_num, log2_max_frame_num);
  sodb += uegc(idr_pic_id); 
  sodb += Bitstream(pic_order_cnt_lsb, log2_max_pic_order_cnt_lsb);
  sodb += Bitstream(no_output_of_prior_pics_flag); 
  sodb += Bitstream(long_term_reference_flag);
  sodb += segc(slice_qp_delta); 
  sodb += uegc(disable_deblocking_filter_idc);

  return sodb;
}