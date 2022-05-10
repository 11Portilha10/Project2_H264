#ifndef FRAME_ENCODE_H
#define FRAME_ENCODE_H

#include <vector>
#include <experimental/optional>
#include <functional>
#include <tuple>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "block.h"
#include "macroblock.h"
#include "frame.h"
#include "intra.h"

using namespace cv;
using namespace std;


int encode_Y_intra16x16_block(MacroBlock&, std::vector<MacroBlock>&, Frame&);
int encode_Y_intra4x4_block(int, MacroBlock&, MacroBlock&, std::vector<MacroBlock>&, Frame&);

int encode_Y_block(MacroBlock&, std::vector<MacroBlock>&, Frame&);

int encode_Cr_Cb_intra8x8_block(MacroBlock&, std::vector<MacroBlock>&, Frame&);

int encode_Cb_Cr_block(MacroBlock&, std::vector<MacroBlock>&, Frame&);

void encode_I_frame(Frame&);


#endif