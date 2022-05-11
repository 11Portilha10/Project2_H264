#include "frame.h"

/* Initialize Frame(I-Picture)
 *
 * Only I-Picture can be initialized with a padded frame, since there is no dependency
 * between I-Picture and other Pictures.
 */
Frame::Frame(const Mat& yuv)
: type(I_PICTURE)
{
  // data structure (raw image) dimensions
  this->raw_height = yuv.rows;
  this->raw_width = yuv.cols;

  // real dimensions of the image
  this->height = (yuv.rows<<1)/3;
  this->width = yuv.cols;

  // width and height SHOULD be multiple of 16
  int hPad = this->width % 16;
  int vPad = this->height % 16;

  // if dimensions are not multiple of 16, throw exception!!!
  if(hPad || vPad)
  {
    // throw exception
    cerr << "Image not padded!" << endl;
  }

  // Basic unit: number of columns, number of rows, number of macroblocks
  this->nb_mb_cols = this->width >> 4;
  this->nb_mb_rows = this->height >> 4;
  uint16_t nb_mbs = this->nb_mb_cols * this->nb_mb_rows;
  uint16_t cnt_mbs = 0;

  // Reserve the capacity of vector
  this->mbs.reserve(nb_mbs);

  uint8_t* pixelPtr = (uint8_t*)yuv.data;                     // pointer to pixel data
  // 179968 pixels for luma
  // 269952 total pixels
  uint32_t u_offset = this->height * this->width;              // offset to U component
  uint32_t v_offset = u_offset + (this->height >> 2)*this->width;  // offset to V component

  // Fill LUMA component for each macroblock (padding NOT considered)
  int y=0, x=0;
  for (y = 0; y < this->nb_mb_rows; y++) {
    for (x = 0; x < this->nb_mb_cols; x++) {

      // Initialize macroblock with row (y) and column (x) address
      MacroBlock mb(y, x);

      // cout << x << " , " << y << endl;

      // Luma component (16x16 pixels)
      for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
        {
          uint32_t index = i*this->width + j + (x<<4) + (y<<4)*this->width;
          mb.Y[(i<<4) + j] = pixelPtr[index];
          //cout << index << endl;
        }

      // Chroma U component (Blue projection) (8x8 pixels)
      for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
          uint32_t index = u_offset + j + (i>>1)*this->width + (i & 0x01)*(this->width>>1) 
                                      + (x<<3)  + (y<<2)*this->width;
          mb.Cb[(i<<3) + j] = pixelPtr[index];
        }

      // Chroma V component (Red projection) (8x8 pixels)
      for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
          uint32_t index = v_offset + j + (i>>1)*this->width + (i & 0x01)*(this->width>>1) 
                                      + (x<<3)  + (y<<2)*this->width;
          mb.Cr[(i<<3) + j] = pixelPtr[index];
          // cout << index << endl;
        }

      // MB index
      mb.mb_index = cnt_mbs++;
      // Push into the vector of macroblocks
      this->mbs.push_back(mb);
    }
  }

}

int Frame::get_neighbor_index(const int curr_index, const int neighbor_type) {
  int neighbor_index = 0;

  switch(neighbor_type) {
    case MB_NEIGHBOR_UL: // Upper left neighbor
      if ((curr_index % this->nb_mb_cols == 0) || (curr_index < this->nb_mb_cols))
        neighbor_index = -1;
      else
        neighbor_index = curr_index - this->nb_mb_cols - 1;
      break;

    case MB_NEIGHBOR_U: // Upper neighbor
      if(curr_index < this->nb_mb_cols)
        neighbor_index = -1;
      else
        neighbor_index = curr_index - this->nb_mb_cols;
      break;

    case MB_NEIGHBOR_UR: // Upper right neighbor
      if (((curr_index + 1) % this->nb_mb_cols == 0) || (curr_index < this->nb_mb_cols))
        neighbor_index = -1;
      else
        neighbor_index = curr_index - this->nb_mb_cols + 1;
      break;

    case MB_NEIGHBOR_L: // Left neighbor
      if (curr_index % this->nb_mb_cols == 0)
        neighbor_index = -1;
      else
        neighbor_index = curr_index - 1;
      break;

    case MB_NEIGHBOR_R: // Right neighbor
      if ((curr_index + 1) % this->nb_mb_cols == 0)
        neighbor_index = -1;
      else
        neighbor_index = curr_index + 1;
      break;

    default:
      neighbor_index = -1;
      break;
  }

  // If neighbor doesn't exist, return -1
  if (neighbor_index < 0)
    neighbor_index = -1;
  return neighbor_index;
}