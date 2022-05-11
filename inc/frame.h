#ifndef FRAME_H
#define FRAME_H

#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "macroblock.h"

using namespace cv;
using namespace std;

enum {
  MB_NEIGHBOR_UL,
  MB_NEIGHBOR_U,
  MB_NEIGHBOR_UR,
  MB_NEIGHBOR_L,
  MB_NEIGHBOR_R
};

enum {
  I_PICTURE,
  P_PICTURE
};

class Frame {
public:

  int type;         // frame type (I, P, B)
  uint16_t width;        // width in pixels with padding
  uint16_t height;       // height in pixels with padding
  uint16_t raw_width;    // width in pixels without padding
  uint16_t raw_height;   // height in pixels without padding
  int nb_mb_rows;   // number of MB rows
  int nb_mb_cols;   // number of MB cols

  std::vector<MacroBlock> mbs;

  Frame(const Mat& yuv);
  int get_neighbor_index(const int, const int);
};

#endif