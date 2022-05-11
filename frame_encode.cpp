#include "frame_encode.h"

// aux flags

int error_4x4=0;
int error_16x16=0;

///////////////////////////////////////////////// 16X16 ///////////////////////////////////////////7

/*
*   Function to apply 16x16 prediction and get the error
*
*/
int encode_Y_intra16x16_block(MacroBlock& mb, std::vector<MacroBlock>& decoded_blocks, Frame& frame) {
  
  static int mb_pred = 0;
  ofstream pred_file ("txt/pred_mode.txt", ios::app);

  // Get neighbours MBs pointers (ul, u, l)
  auto get_decoded_Y_block = [&](int direction) {
    int index = frame.get_neighbor_index(mb.mb_index, direction);   // works well
    if (index == -1)
      return std::experimental::optional<std::reference_wrapper<Block16x16>>(); // If there is no neighbours it doesnt return initialized mb
    else
      return std::experimental::optional<std::reference_wrapper<Block16x16>>(decoded_blocks.at(index).Y);
  };

  // Apply intra prediction
  int error;
  Intra16x16Mode mode;

  //Inputs Y mb and neighbours obtained from above function
  std::tie(error, mode) = intra16x16(mb.Y, get_decoded_Y_block(MB_NEIGHBOR_UL),
                                           get_decoded_Y_block(MB_NEIGHBOR_U),
                                           get_decoded_Y_block(MB_NEIGHBOR_L));

  // Sets 16x16 prediction flag
  mb.is_intra16x16 = true;

  // Sets 16x16 mode
  mb.intra16x16_Y_mode = mode;

  // Print prediction mode to 'pred_mode.txt'
  pred_file << "MB " << mb_pred << " ->" << (int)mode << endl;
  mb_pred++;

  // Perform QDCT
  qdct_luma16x16_intra(mb.Y);
  
  // Reconstruct for later prediction (not being done)

  return error;
}



/*
*   Function to apply 4x4 prediction and get the error
*
*/
int encode_Y_intra4x4_block(int cur_pos, MacroBlock& mb, MacroBlock& decoded_block, std::vector<MacroBlock>& decoded_blocks, Frame& frame) {
  int temp_pos = MacroBlock::convert_table[cur_pos];

  auto get_4x4_block = [&](int index, int pos) {
    if (index == -1)
      return std::experimental::optional<Block4x4>();
    else if (index == mb.mb_index)
      return std::experimental::optional<Block4x4>(decoded_block.get_Y_4x4_block(pos));
    else
      return std::experimental::optional<Block4x4>(decoded_blocks.at(index).get_Y_4x4_block(pos));
  };

  // Gets upper left 4x4 block
  auto get_UL_4x4_block = [&]() {
    int index, pos;
    if (temp_pos == 0) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_UL);
      pos = 15;
    } else if (1 <= temp_pos && temp_pos <= 3) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
      pos = 11 + temp_pos;
    } else if (temp_pos % 4 == 0) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
      pos = temp_pos - 1;
    } else {
      index = mb.mb_index;
      pos = temp_pos - 5;
    }

    return get_4x4_block(index, MacroBlock::convert_table[pos]);
  };

  // Gets upper 4x4 block
  auto get_U_4x4_block = [&]() {
    int index, pos;
    if (0 <= temp_pos && temp_pos <= 3) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
      pos = 12 + temp_pos;
    } else {
      index = mb.mb_index;
      pos = temp_pos - 4;
    }

    return get_4x4_block(index, MacroBlock::convert_table[pos]);
  };

  // Gets upper right 4x4 block
  auto get_UR_4x4_block = [&]() {
    int index, pos;
    if (temp_pos == 3) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_UR);
      pos = 12;
    } else if (temp_pos == 5 || temp_pos == 13) {
      index = -1;
      pos = 0;
    } else if (0 <= temp_pos && temp_pos <= 2) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
      pos = 1 + temp_pos;
    } else if ((temp_pos + 1) % 4 == 0) {
      index = -1;
      pos = 0;
    } else {
      index = mb.mb_index;
      pos = temp_pos - 3;
    }

    return get_4x4_block(index, MacroBlock::convert_table[pos]);
  };

  // Gets left 4x4 block
  auto get_L_4x4_block = [&]() {
    int index, pos;
    if (temp_pos % 4 == 0) {
      index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
      pos = temp_pos + 3;
    } else {
      index = mb.mb_index;
      pos = temp_pos - 1;
    }

    return get_4x4_block(index, MacroBlock::convert_table[pos]);
  };

  int error = 0;
  Intra4x4Mode mode;
  std::tie(error, mode) = intra4x4(mb.get_Y_4x4_block(cur_pos),
                                   get_UL_4x4_block(),
                                   get_U_4x4_block(),
                                   get_UR_4x4_block(),
                                   get_L_4x4_block());

  mb.is_intra16x16 = false;
  mb.intra4x4_Y_mode.at(cur_pos) = mode;

  // Perform QDCT
  qdct_luma4x4_intra(mb.get_Y_4x4_block(cur_pos));


  // Reconstruct for later prediction

  return error;
}


/*
*   Function to encode 16x16 Y block, comparing 4x4 and 16x16 prediction errors
*
*/
int encode_Y_block(MacroBlock& mb, std::vector<MacroBlock>& decoded_blocks, Frame& frame) {
  // Temp marcoblock for choosing two predicitons
  MacroBlock temp_block = mb;
  MacroBlock temp_decoded_block = mb;

  // Perform intra16x16 prediction
  int error_intra16x16 = encode_Y_intra16x16_block(mb, decoded_blocks, frame);

  // Perform intra4x4 prediction
  int error_intra4x4 = 0;
  for (int i = 0; i != 16; i++)
    error_intra4x4 += encode_Y_intra4x4_block(i, temp_block, temp_decoded_block, decoded_blocks, frame);

  // compare the error of two predictions
  // if (error_intra4x4 < error_intra16x16) -> condiçao sempre falsa?
  if (false && error_intra4x4 < error_intra16x16){
    mb = temp_block;
    decoded_blocks.at(mb.mb_index) = temp_decoded_block;

    error_4x4=1;
    return error_intra4x4;
  }
  
  else 
  {
    error_16x16=1;
    return error_intra16x16;
  }
}

//////////////////////////////////////////////// 8X8 ////////////////////////////////////////////////7


/*
*   Function to apply 8x8 prediction and get the error
*
*/
int encode_Cr_Cb_intra8x8_block(MacroBlock& mb, std::vector<MacroBlock>& decoded_blocks, Frame& frame) {
  auto get_decoded_Cr_block = [&](int direction) {
    int index = frame.get_neighbor_index(mb.mb_index, direction);
    if (index == -1)
      return std::experimental::optional<std::reference_wrapper<Block8x8>>();
    else
      return std::experimental::optional<std::reference_wrapper<Block8x8>>(decoded_blocks.at(index).Cr);
  };

  auto get_decoded_Cb_block = [&](int direction) {
    int index = frame.get_neighbor_index(mb.mb_index, direction);
    if (index == -1)
      return std::experimental::optional<std::reference_wrapper<Block8x8>>();
    else
      return std::experimental::optional<std::reference_wrapper<Block8x8>>(decoded_blocks.at(index).Cb);
  };

  int error;
  IntraChromaMode mode;
  std::tie(error, mode) = intra8x8_chroma(mb.Cr, get_decoded_Cr_block(MB_NEIGHBOR_UL),
                                                 get_decoded_Cr_block(MB_NEIGHBOR_U),
                                                 get_decoded_Cr_block(MB_NEIGHBOR_L),
                                          mb.Cb, get_decoded_Cb_block(MB_NEIGHBOR_UL),
                                                 get_decoded_Cb_block(MB_NEIGHBOR_U),
                                                 get_decoded_Cb_block(MB_NEIGHBOR_L));

  mb.intra_Cr_Cb_mode = mode;

  // Perform QDCT (Cr and Cb components)
  qdct_chroma8x8_intra(mb.Cr);
  qdct_chroma8x8_intra(mb.Cb);


  // Reconstruct for later prediction
  
  return error;
}


/*
*   Function to encode 8x8 Cr and Cb blocks
*
*/
int encode_Cb_Cr_block(MacroBlock& mb, std::vector<MacroBlock>& decoded_blocks, Frame& frame) {
  
  int error_intra8x8 = encode_Cr_Cb_intra8x8_block(mb, decoded_blocks, frame);
 
  return error_intra8x8;
}

/////////////////////////////////////////////////////////////////////// FRAME ////////////////////////////////


/*
*   Function to encode all frame (composed by Y, Cr and Cb)
*
*/

void encode_I_frame(Frame& frame) {
  // decoded Y blocks for intra prediction
  std::vector<MacroBlock> decoded_blocks;
  decoded_blocks.reserve(frame.mbs.size());

  ofstream error_file ("txt/errors.txt", ios::out);
  ofstream mb_input_file ("txt/mb_input.txt", ios::out);
  ofstream mb_output_file ("txt/mb_output.txt", ios::out);
  int error_cnt=0;
  int mb_cnt_in=0;
  int mb_cnt_out=0;

  cout << "Number of Macroblocks:" << frame.mbs.size() << endl;

  // Loops through all MB
  for (auto& mb : frame.mbs) {

    decoded_blocks.push_back(mb);   // vector to reconstruct macroblocks
    MacroBlock origin_block = mb;

    // Print all 703 Macroblock Y (16x16) component to 'mb_input.txt' 
    mb_input_file << "Y_MB input " << mb_cnt_in << endl; 
    for(int rows=0; rows < 256; rows+=16)
    {
      for(int cols=0; cols<16; cols++)
      {
        mb_input_file << mb.Y[rows+cols] << ' ';
      }
      mb_input_file << endl;
    }
    mb_cnt_in++;
    mb_input_file << endl;

    // Encode Luma component, output is in 'mb.Y vector'
    int error_luma = encode_Y_block(mb, decoded_blocks, frame);

    // Print all 703 Macroblock Y (16x16) component after prediction to 'mb_output.txt' 
    mb_output_file << "Y_MB output " << mb_cnt_out << endl; 
    for(int r=0; r < 256; r+=16)
    {
      for(int c=0; c<16; c++)
      {
        mb_output_file << mb.Y[r+c] << ' ';
      }
      mb_output_file << endl;
    }
    mb_cnt_out++;
    mb_output_file << endl;

    // Encoding Chroma component function
    int error_chroma = encode_Cb_Cr_block(mb, decoded_blocks, frame);

    // Print to 'errors.txt' the prediction block size (4x4 or 16x16), luma and chroma errors (SADs)
    error_file << "MB" << error_cnt << " ->";
    if(error_4x4)
      error_file << "Error type:4x4" << ' ';
    else if(error_16x16)
      error_file << "Error type:16x16" << ' ';
    error_file << "Luma error:" << error_luma << ' ' << ' ';
    error_file << "Chroma error:" << error_chroma << endl;
    error_4x4=0;    // clear pred type flags and increment counter
    error_16x16=0;
    error_cnt++;

    // Defined threshold for bad predictions, if SAD is greater MB remains the same
    if (error_luma > 2000 || error_chroma > 1000) {
      mb = origin_block;
      decoded_blocks.back() = origin_block;
      mb.is_I_PCM = true;   // what does this means?
    }
  }
}