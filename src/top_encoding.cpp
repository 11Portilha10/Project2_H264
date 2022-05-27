#include "top_encoding.h"

/**
 * @brief 
 * 
 */
void vlc_frame(Frame& frame) {

  // vector of nMB arrays of 16 ints for Luma, 4 for Chroma
  // each macroblock has an array of 16 ints (each 4x4 T. block needs to count non-zero coeffs)
  std::vector<std::array<int, 16>> nc_Y_table;
  nc_Y_table.reserve(frame.mbs.size());           
  std::vector<std::array<int, 4>> nc_Cb_table;
  nc_Cb_table.reserve(frame.mbs.size());
  std::vector<std::array<int, 4>> nc_Cr_table;
  nc_Cr_table.reserve(frame.mbs.size());

  // int mb_no = 0;
  for (auto& mb : frame.mbs) {
    // f_logger.log(Level::DEBUG, "mb #" + std::to_string(mb_no++));

    // Fill current tables
    std::array<int, 16> current_Y_table;  // 16x16 Y => 16 4x4 T. blocks
    std::array<int, 4> current_Cb_table;  // 8x8 Cb => 4 4x4 T. blocks
    std::array<int, 4> current_Cr_table;  // 8x8 Cr => 4 4x4 T. blocks
    nc_Y_table.push_back(current_Y_table);
    nc_Cb_table.push_back(current_Cb_table);
    nc_Cr_table.push_back(current_Cr_table);

    // If the MB is not coded, fill YCbCr tables with 16 (because all 4x4 blocks have 16 non_zero coeffs)
    if (mb.is_I_PCM) {
      for (int i = 0; i != 16; i++)
        nc_Y_table.at(mb.mb_index)[i] = 16;
      for (int i = 0; i != 4; i++) {
        nc_Cb_table.at(mb.mb_index)[i] = 16;
        nc_Cr_table.at(mb.mb_index)[i] = 16;
      }

      continue;
    }

    // For intra 16x16 coded MBs, there is a 4x4 DC coeff transform block 
    if (mb.is_intra16x16)
      mb.bitstream += vlc_Y_DC(mb, nc_Y_table, frame);  // add DC block bitstream to MB bitstream

    std::array<Bitstream, 4> temp_luma;

    // Encode all 16 AC T. coeffs (concatenate for each 4 blocks)
    for (int i = 0; i != 16; i++)
      temp_luma[i / 4] += vlc_Y(i, mb, nc_Y_table, frame);
    if (mb.is_intra16x16) {
      if (mb.coded_block_pattern_luma)  // if the whole MB has non-zero coeffs...
        for (int i = 0; i != 4; i++)
          mb.bitstream += temp_luma[i];
    } else {
      for (int i = 0; i != 4; i++)
        if (mb.coded_block_pattern_luma_4x4[i])   // if the MB 8x8 sub-block has non-zero coeffs...
          mb.bitstream += temp_luma[i];
    }

    Bitstream temp_chroma_DC;   // for DC T. coeff block
    Bitstream temp_chroma_AC;   // for all 4 4x4 AC T. coeff blocks

    // Each chroma component of a MB has a 2x2 DC T. coeff block
    temp_chroma_DC += vlc_Cb_DC(mb);
    temp_chroma_DC += vlc_Cr_DC(mb);

    // Encode all 4 AC T. coeff blocks
    for (int i = 0; i != 4; i++)
      temp_chroma_AC += vlc_Cb_AC(i, mb, nc_Cb_table, frame);
    for (int i = 0; i != 4; i++)
      temp_chroma_AC += vlc_Cr_AC(i, mb, nc_Cr_table, frame);

    if (mb.coded_block_pattern_chroma_DC || mb.coded_block_pattern_chroma_AC) // if DC T. coeffs block has non-zero coeffs...
      mb.bitstream += temp_chroma_DC;
    if (mb.coded_block_pattern_chroma_AC)   // if any of the 4 AC T. coeffs block has non-zero coeffs
      mb.bitstream += temp_chroma_AC;
  }
}


////////////////////////////////////////////////////// LUMA ///////////////////////////////////////////////////////


/**
 * @brief Encodes the 4x4 Luma DC Transform coefficients block
 * 
 * @param mb The Macroblock to be processed
 * @param nc_Y_table Table of number of non-zero coefficients for Luma MBs
 * @param frame The frame being processed
 * 
 * @return Coded bitstream of the 4x4 Luma DC coeffs block
 */
Bitstream vlc_Y_DC(MacroBlock& mb, std::vector<std::array<int, 16>>& nc_Y_table, Frame& frame) {
  int nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);  // get left MB index
  int nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);  // get upper MB index

  // WHICH ARE THE NEIGHBOUR BLOCKS OF THE DC BLOCK ???
  int nC;
  if (nA_index != -1 && nB_index != -1)   // both available
    nC = (nc_Y_table.at(nA_index)[5] + nc_Y_table.at(nB_index)[10] + 1) >> 1;
  else if (nA_index != -1)                // left available
    nC = nc_Y_table.at(nA_index)[5];
  else if (nB_index != -1)                // upper available
    nC = nc_Y_table.at(nB_index)[10];
  else                                    // both unavailable
    nC = 0;

  Bitstream bitstream;
  int non_zero;   // does the number of non_zero coeffs apply for DC blocks ???
  std::tie(bitstream, non_zero) = cavlc_block4x4(mb.get_Y_DC_block(), nC, 16);

  return bitstream;
}

/**
 * @brief Encodes the 4x4 Luma AC Transform Coefficient block in position 'cur_pos'
 * 
 * @param cur_pos Current 4x4 block position within the MB
 * @param mb Current MB being processed
 * @param nc_Y_table Table of number of non-zero coeffs for Luma MBs
 * @param frame description
 * 
 * @return description of the return value
 */
Bitstream vlc_Y(int cur_pos, MacroBlock& mb, std::vector<std::array<int, 16>>& nc_Y_table, Frame& frame) {
  int real_pos = MacroBlock::convert_table[cur_pos];

  int nA_index, nA_pos;
  if (real_pos % 4 == 0) {  // left 4x4 block in the left MB
    nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
    nA_pos = real_pos + 3;
  } else {                  // left 4x4 block in the same MB
    nA_index = mb.mb_index;
    nA_pos = real_pos - 1;
  }
  nA_pos = MacroBlock::convert_table[nA_pos];

  int nB_index, nB_pos;
  if (0 <= real_pos && real_pos <= 3) {   // upper 4x4 block in the upper MB
    nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
    nB_pos = 12 + real_pos;
  } else {                                // upper 4x4 block in the same MB
    nB_index = mb.mb_index;
    nB_pos = real_pos - 4;
  }
  nB_pos = MacroBlock::convert_table[nB_pos];

  int nC;
  if (nA_index != -1 && nB_index != -1)
    nC = (nc_Y_table.at(nA_index)[nA_pos] + nc_Y_table.at(nB_index)[nB_pos] + 1) >> 1;
  else if (nA_index != -1)
    nC = nc_Y_table.at(nA_index)[nA_pos];
  else if (nB_index != -1)
    nC = nc_Y_table.at(nB_index)[nB_pos];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  if (mb.is_intra16x16)   // if intra 16x16 coded, a DC 4x4 transform was applied to all 16 DC coeffs
    std::tie(bitstream, non_zero) = cavlc_block4x4(mb.get_Y_AC_block(cur_pos), nC, 15);
  else                    // if not, only default 4x4 transform was applied
    std::tie(bitstream, non_zero) = cavlc_block4x4(mb.get_Y_4x4_block(cur_pos), nC, 16);
  
  // Save number of non-zero coeffs for further nC choices
  nc_Y_table.at(mb.mb_index)[cur_pos] = non_zero;

  // Set flags to indicate that the corresponding block/MB has non-zero coeffs
  if (non_zero != 0) {
    mb.coded_block_pattern_luma = true;   // for the whole MB
    mb.coded_block_pattern_luma_4x4[cur_pos / 4] = true;  // for the corresponding 4x4 block
  }

  return bitstream;
}



/////////////////////////////////////////// CHROMA ///////////////////////////////////////////////



/**
 * @brief Performs encoding of the 2x2 DC coeffs block for Cb
 * 
 * @param mb The MB being processed
 * 
 * @return Bitstream of the encoded DC block of coeffs
 */
Bitstream vlc_Cb_DC(MacroBlock& mb) {
  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero) = cavlc_block2x2(mb.get_Cb_DC_block(), -1, 4);

  if (non_zero != 0)
    mb.coded_block_pattern_chroma_DC = true;

  return bitstream;
}

/**
 * @brief Performs encoding of the 2x2 DC coeffs block for Cr
 * 
 * @param mb The MB being processed
 * 
 * @return Bitstream of the encoded DC block of coeffs
 */
Bitstream vlc_Cr_DC(MacroBlock& mb) {
  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero) = cavlc_block2x2(mb.get_Cr_DC_block(), -1, 4);

  if (non_zero != 0)
    mb.coded_block_pattern_chroma_DC = true;

  return bitstream;
}

/**
 * @brief Performs encoding of a 4x4 T. coeff block of the MB's Cb component
 * 
 * @param cur_pos Position of the 4x4 block within the Cb component
 * @param mb The MB being processed
 * @param nc_Cb_table Table of number of non-zero coeffs for Cb blocks
 * @param frame The frame being processed
 * 
 * @return Encoded bitstream of the 4x4 AC block indexed with 'cur_pos'
 */
Bitstream vlc_Cb_AC(int cur_pos, MacroBlock& mb, std::vector<std::array<int, 4>>& nc_Cb_table, Frame& frame) {
  int nA_index, nA_pos;

  // Same as Luma for left block, but with 4 4x4 blocks
  if (cur_pos % 2 == 0) {
    nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
    nA_pos = cur_pos + 1;
  } else {
    nA_index = mb.mb_index;
    nA_pos = cur_pos - 1;
  }

  // Same as Luma for upper block, but with 4 4x4 blocks
  int nB_index, nB_pos;
  if (0 <= cur_pos && cur_pos <= 1) {
    nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
    nB_pos = cur_pos + 2;
  } else {
    nB_index = mb.mb_index;
    nB_pos = cur_pos - 2;
  }

  int nC;
  if (nA_index != -1 && nB_index != -1)
    nC = (nc_Cb_table.at(nA_index)[nA_pos] + nc_Cb_table.at(nB_index)[nB_pos] + 1) >> 1;
  else if (nA_index != -1)
    nC = nc_Cb_table.at(nA_index)[nA_pos];
  else if (nB_index != -1)
    nC = nc_Cb_table.at(nB_index)[nB_pos];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero) = cavlc_block4x4(mb.get_Cb_AC_block(cur_pos), nC, 15);
  // Save number of non-zero coeffs for further nC choices
  nc_Cb_table.at(mb.mb_index)[cur_pos] = non_zero;

  if (non_zero != 0)
    mb.coded_block_pattern_chroma_AC = true;  // set flag to indicate that non-zero coeffs are present

  return bitstream;
}

/**
 * @brief Performs encoding of a 4x4 T. coeff block of the MB's Cr component
 * 
 * @param cur_pos Position of the 4x4 block within the Cr component
 * @param mb The MB being processed
 * @param nc_Cb_table Table of number of non-zero coeffs for Cr blocks
 * @param frame The frame being processed
 * 
 * @return Encoded bitstream of the 4x4 AC block indexed with 'cur_pos'
 */
Bitstream vlc_Cr_AC(int cur_pos, MacroBlock& mb, std::vector<std::array<int, 4>>& nc_Cr_table, Frame& frame) {
  int nA_index, nA_pos;
  if (cur_pos % 2 == 0) {
    nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
    nA_pos = cur_pos + 1;
  } else {
    nA_index = mb.mb_index;
    nA_pos = cur_pos - 1;
  }

  int nB_index, nB_pos;
  if (0 <= cur_pos && cur_pos <= 1) {
    nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
    nB_pos = cur_pos + 2;
  } else {
    nB_index = mb.mb_index;
    nB_pos = cur_pos - 2;
  }

  int nC;
  if (nA_index != -1 && nB_index != -1)
    nC = (nc_Cr_table.at(nA_index)[nA_pos] + nc_Cr_table.at(nB_index)[nB_pos] + 1) >> 1;
  else if (nA_index != -1)
    nC = nc_Cr_table.at(nA_index)[nA_pos];
  else if (nB_index != -1)
    nC = nc_Cr_table.at(nB_index)[nB_pos];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero) = cavlc_block4x4(mb.get_Cr_AC_block(cur_pos), nC, 15);
  nc_Cr_table.at(mb.mb_index)[cur_pos] = non_zero;

  if (non_zero != 0)
    mb.coded_block_pattern_chroma_AC = true;

  return bitstream;
}
