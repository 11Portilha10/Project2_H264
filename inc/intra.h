#ifndef INTRA_H_
#define INTRA_H_

#include <array>
#include <vector>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <functional>
#include <experimental/optional>
#include <iostream>
#include <fstream>
#include <iostream>
#include <type_traits>

#include "block.h"

using namespace std;

class Predictor {
public:
    std::vector<int> pred_pel;
    bool up_available;
    bool left_available;
    bool up_right_available;
    bool all_available;
    
    // Allocates space for the predictors relating the block size (e.g 4x4 block->13 predictors (A-Q))

    Predictor(int size): up_available(false), left_available(false), up_right_available(false), all_available(false) {
      switch (size) {
        case 4:
          this->pred_pel.reserve(13);
          break;
        case 8:
          this->pred_pel.reserve(17);
          break;
        case 16:
          this->pred_pel.reserve(33);
          break;
      }
    }
};

// std::ostream& operator << (std::ostream& os, const Intra16x16Mode& obj)
// {
//    os << static_cast<std::underlying_type<Intra16x16Mode>::type>(obj);
//    return os;
// }

using CopyBlock4x4 = std::array<int, 16>;

// 9 predictions modes -> 4x4 Luma
enum class Intra4x4Mode {
  VERTICAL,
  HORIZONTAL,
  DC,
  DOWNLEFT,
  DOWNRIGHT,
  VERTICALRIGHT,
  HORIZONTALDOWN,
  VERTICALLEFT,
  HORIZONTALUP,
};

// 4 prediction modes -> 16x16 Luma 
enum class Intra16x16Mode {
  VERTICAL,
  HORIZONTAL,
  DC,
  PLANE
};


// 4 predictions modes -> 8x8 Chroma
enum class IntraChromaMode {
  DC,
  HORIZONTAL,
  VERTICAL,
  PLANE
};


//A tuple is an object capable to hold a collection of elements. Each element can be of a different type.
//The class template std::experimental::optional manages an optional contained value, i.e. a value that semantically may not be present.  



////////////////////// 4x4 MODES ////////////////////////

std::tuple<int, Intra4x4Mode> intra4x4(Block4x4, std::experimental::optional<Block4x4>, 
                                                 std::experimental::optional<Block4x4>,
                                                 std::experimental::optional<Block4x4>, 
                                                 std::experimental::optional<Block4x4>);

void get_intra4x4(CopyBlock4x4&, const Predictor&, const Intra4x4Mode);
void intra4x4_vertical(CopyBlock4x4&, const Predictor&);
void intra4x4_horizontal(CopyBlock4x4&, const Predictor&);
void intra4x4_dc(CopyBlock4x4&, const Predictor&);
void intra4x4_downleft(CopyBlock4x4&, const Predictor&);
void intra4x4_downright(CopyBlock4x4&, const Predictor&);
void intra4x4_verticalright(CopyBlock4x4&, const Predictor&);
void intra4x4_horizontaldown(CopyBlock4x4&, const Predictor&);
void intra4x4_verticalleft(CopyBlock4x4&, const Predictor&);
void intra4x4_horizontalup(CopyBlock4x4&, const Predictor&);

Predictor get_intra4x4_predictor(std::experimental::optional<Block4x4>, std::experimental::optional<Block4x4>,
                                 std::experimental::optional<Block4x4>, std::experimental::optional<Block4x4>);
                                  

////////////////////// 16x16 MODES ////////////////////////


std::tuple<int, Intra16x16Mode> intra16x16(Block16x16&, std::experimental::optional<std::reference_wrapper<Block16x16>>, 
                                                        std::experimental::optional<std::reference_wrapper<Block16x16>>, 
                                                        std::experimental::optional<std::reference_wrapper<Block16x16>>);

void get_intra16x16(Block16x16&, const Predictor&, const Intra16x16Mode);
void intra16x16_vertical(Block16x16&, const Predictor&);
void intra16x16_horizontal(Block16x16&, const Predictor&);
void intra16x16_dc(Block16x16&, const Predictor&);
void intra16x16_plane(Block16x16&, const Predictor&);

Predictor get_intra16x16_predictor(std::experimental::optional<std::reference_wrapper<Block16x16>>, std::experimental::optional<std::reference_wrapper<Block16x16>>, 
                                   std::experimental::optional<std::reference_wrapper<Block16x16>>);


////////////////////// 8x8 MODES ////////////////////////

std::tuple<int, IntraChromaMode> intra8x8_chroma(Block8x8&, std::experimental::optional<std::reference_wrapper<Block8x8>>, 
                                                            std::experimental::optional<std::reference_wrapper<Block8x8>>, 
                                                            std::experimental::optional<std::reference_wrapper<Block8x8>>,
                                                
                                                 Block8x8&, std::experimental::optional<std::reference_wrapper<Block8x8>>, 
                                                            std::experimental::optional<std::reference_wrapper<Block8x8>>, 
                                                            std::experimental::optional<std::reference_wrapper<Block8x8>>);

void get_intra8x8_chroma(Block8x8&, const Predictor&, const IntraChromaMode);
void intra8x8_chroma_dc(Block8x8&, const Predictor&);
void intra8x8_chroma_horizontal(Block8x8&, const Predictor&);
void intra8x8_chroma_vertical(Block8x8&, const Predictor&);
void intra8x8_chroma_plane(Block8x8&, const Predictor&);

Predictor get_intra8x8_chroma_predictor(std::experimental::optional<std::reference_wrapper<Block8x8>>, 
                                        std::experimental::optional<std::reference_wrapper<Block8x8>>, 
                                        std::experimental::optional<std::reference_wrapper<Block8x8>>);


#endif