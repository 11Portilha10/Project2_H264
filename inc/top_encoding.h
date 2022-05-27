#ifndef TOP_ENC_H_
#define TOP_ENC_H_

#include <array>
#include <vector>
#include <tuple>

#include "frame.h"
#include "macroblock.h"
#include "vlc.h"

void vlc_frame(Frame&);
Bitstream vlc_Y_DC(MacroBlock&, std::vector<std::array<int, 16>>&, Frame&);
Bitstream vlc_Y(int, MacroBlock&, std::vector<std::array<int, 16>>&, Frame&);
Bitstream vlc_Cb_DC(MacroBlock&);
Bitstream vlc_Cr_DC(MacroBlock&);
Bitstream vlc_Cb_AC(int, MacroBlock&, std::vector<std::array<int, 4>>&, Frame&);
Bitstream vlc_Cr_AC(int, MacroBlock&, std::vector<std::array<int, 4>>&, Frame&);

#endif