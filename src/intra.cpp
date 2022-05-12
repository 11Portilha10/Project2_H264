#include "intra.h"

/* Clip function for 16x16 plane prediction
* Returns max value between lower and (min(n,upper))
*/
template <typename T>
T clip(const T& n, const T& lower, const T& upper)
{
  return std::max(lower, std::min(n, upper));
}

/* Input iterators; return summation of absolute difference (SAD)
   SAD =  SUM |source_pixel - predicted_pixel| (for all pixels in MB)
*/
template<class InputIt1, class InputIt2, class OutputIt>
int SAD(InputIt1 first1, InputIt1 last1, InputIt2 first2, OutputIt result) 
{
  int sad = 0;
  int diff;
  while (first1 != last1)  //goes through all MB pixels
  {
    diff = (*first1++ - *first2++); // subtracts source pixel and predicted pixel and increment iterator
    *result++ = diff;               // MAYBE HERE IS WHERE THE DIFFERENCE IS BEING CALCULATED
    sad += (diff > 0)? diff: -diff; 
  }
  return sad;
}

////////////////////////////////////////////////////////// 4x4 MODES //////////////////////////////////////////////////////

/* Input 4x4 block and its neighbors
 * Do intra4x4 prediction which has 9 modes
 * Overwrite residual on input block
 * Return the least cost mode
 * 
 * Consult predicion.png
 */

// Current MB and 4 neighbours
std::tuple<int, Intra4x4Mode> intra4x4(Block4x4 block, std::experimental::optional<Block4x4> ul, std::experimental::optional<Block4x4> u,
                                                       std::experimental::optional<Block4x4> ur, std::experimental::optional<Block4x4> l) {

  ofstream myfile ("txt/4x4_Y_predictors.txt", ios::app);
  static int predictor_cnt=0;

  // Get predictors
  Predictor predictor = get_intra4x4_predictor(ul, u, ur, l);

  // Print predictors to '16x16predictors.txt'
  myfile << "MB " << predictor_cnt/16 << " (" << predictor_cnt%16 << ") ->";
  for (int i = 0; i < 13; i++)
  {
    myfile << predictor.pred_pel[i] << ' ';
  }
  myfile << endl;
  predictor_cnt++;

  int mode;
  Intra4x4Mode best_mode;
  CopyBlock4x4 pred, best_pred, residual, best_residual;
  int min_sad = (1 << 15), sad;   // worst SAD is 32768 -> 16*16 pixels = 256; worst prediction = 128-0 ; 256*128 = 32768

  // Run all modes to get least residual
  // Checks if its possible to run prediction mode based on neighbours 
  for (mode = 0; mode < 9; mode++) {

    if ((!predictor.up_available   && (Intra4x4Mode::VERTICAL   == static_cast<Intra4x4Mode>(mode))) ||
        (!predictor.left_available && (Intra4x4Mode::HORIZONTAL == static_cast<Intra4x4Mode>(mode))) ||
        ((!predictor.up_available || !predictor.up_right_available) && (Intra4x4Mode::DOWNLEFT == static_cast<Intra4x4Mode>(mode))) ||
        ((!predictor.up_available || !predictor.left_available) && (Intra4x4Mode::DOWNRIGHT == static_cast<Intra4x4Mode>(mode))) ||
        ((!predictor.up_available || !predictor.left_available) && (Intra4x4Mode::VERTICALRIGHT == static_cast<Intra4x4Mode>(mode))) ||
        ((!predictor.up_available || !predictor.left_available) && (Intra4x4Mode::HORIZONTALDOWN == static_cast<Intra4x4Mode>(mode))) ||
        ((!predictor.up_available || !predictor.up_right_available) && (Intra4x4Mode::VERTICALLEFT == static_cast<Intra4x4Mode>(mode))) ||
        (!predictor.left_available && (Intra4x4Mode::HORIZONTALUP == static_cast<Intra4x4Mode>(mode)))) {
      continue;
    }

    // Aplies prediction
    get_intra4x4(pred, predictor, static_cast<Intra4x4Mode>(mode));

    // Computes SAD and gets best mode
    sad = SAD(block.begin(), block.end(), pred.begin(), residual.begin());
    if (sad < min_sad) {
      min_sad = sad;
      best_mode = static_cast<Intra4x4Mode>(mode);
      std::copy(pred.begin(), pred.end(), best_pred.begin());  // save best predicted block
      std::copy(residual.begin(), residual.end(), best_residual.begin());   // save best residual
    }
  }

  // use operator = instead of std::copy which use *iter to deal with assignment
  for (int i = 0; i < 16; i++) {
    block[i] = best_pred[i];             // Overwirte input block with best predicted (TESTING)
  }

  // // use operator = instead of std::copy which use *iter to deal with assignment
  // for (int i = 0; i < 16; i++) {
  //   block[i] = best_residual[i];        // Overwirte input block with residual
  // }

  // Creates tuple with min SAD and best prediction mode for current MB
  return std::make_tuple(min_sad, best_mode);
}


//Input 4x4 predictors and mode
void get_intra4x4(CopyBlock4x4& pred, const Predictor& p, const Intra4x4Mode mode) {
  switch (mode) {
    case Intra4x4Mode::VERTICAL:
      intra4x4_vertical(pred, p);
      break;
    case Intra4x4Mode::HORIZONTAL:
      intra4x4_horizontal(pred, p);
      break;
    case Intra4x4Mode::DC:
      intra4x4_dc(pred, p);
      break;
    case Intra4x4Mode::DOWNLEFT:
      intra4x4_downleft(pred, p);
      break;
    case Intra4x4Mode::DOWNRIGHT:
      intra4x4_downright(pred, p);
      break;
    case Intra4x4Mode::VERTICALRIGHT:
      intra4x4_verticalright(pred, p);
      break;
    case Intra4x4Mode::HORIZONTALDOWN:
      intra4x4_horizontaldown(pred, p);
      break;
    case Intra4x4Mode::VERTICALLEFT:
      intra4x4_verticalleft(pred, p);
      break;
    case Intra4x4Mode::HORIZONTALUP:
      intra4x4_horizontalup(pred, p);
      break;
  }
}


/*
                            0  1  2  3  4  5  6  7  8  
Q A B C D E F G H           
I 0 0 0 0              9    0  1  2  3
J 0 0 0 0         ->  10    4  5  6  7
K 0 0 0 0             11    8  9  10 11
L 0 0 0 0             12    12 13 14 15

A -> Q are the predictors
0's are the 4x4 block to predict

*/


/* 
Vertical Prediction -> 0,4,8,12 = A
                       1,5,9,13 = B  
                       2,6,10,14 = C 
                       3,7,11,15 = D
*/
void intra4x4_vertical(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int i;
  for (i = 0; i < 4; i++) {
    std::copy_n(p.begin()+1, 4, pred.begin()+i*4);
  }
}


/*
Horizontal Prediction -> 0,1,2,3 = I 
                         4,5,6,7 = J  
                         8,9,10,11 = K 
                         12,13,14,15 = L
*/
void intra4x4_horizontal(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int i, j;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      pred[i*4+j] = p[9+i];
    }
  }
}

// DC Prediction -> 0-15 = (A+B+C+D+I+J+K+L+4)/8 
void intra4x4_dc(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int s1 = 0, s2 = 0, s = 0;
  int i;

  // s1 = A+B+C+D
  for (i = 1; i < 5; i++)
  {
    s1 += p[i];
  }

  // s2 = I+J+K+L
  for (i = 9; i < 13; i++) 
  {
    s2 += p[i];
  }

  // If top predictor (A,B,C,D) and left predictor (I,J,K,L) are avaliable
  if (predictor.up_available && predictor.left_available)
  {
    s = s1 + s2;
  }
  
  // If top predictor is not avaliable
  else if (!predictor.up_available && predictor.left_available) 
  {
    s = 2 * s2;
  }

  // If left predictor is not avaliable
  else if (predictor.up_available && !predictor.left_available) 
  {
    s = 2 * s1;
  }

  // Adds 4 and divides by 8 (shift 3)
  s += 4;
  s >>= 3;

  // If predictors are not avaliable (e.g top left 4x4 block) assumes all predictors=128 
  if (!predictor.up_available && !predictor.left_available) {
    s = 128;
  }

  pred.fill(s);
}

/*
Down-Left Prediction -> 0 = (A+2B+C+2)/4 
                        1,4 = (B+2C+D+2)/4  
                        2,5,8 = (C+2D+E+2)/4 
                        3,6,9,12 = (D+2E+F+2)/4
                        7,10,13 = (E+2F+G+2)/4
                        11,14 = (F+2G+H+2)/4
                        15 = (G+3H+2)/4
*/
void intra4x4_downleft(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;

  pred[0]  = ((p[1] + (p[2] << 1)+ p[3] + 2) >> 2);
  pred[1]  = pred[4]  = ((p[2] + (p[3] << 1) + p[4] + 2) >> 2);
  pred[2]  = pred[5]  = pred[8]  = ((p[3] + (p[4] << 1) + p[5] + 2) >> 2);
  pred[3]  = pred[6]  = pred[9]  = pred[12] = ((p[4] + (p[5] << 1) + p[6] + 2) >> 2);
  pred[7]  = pred[10] = pred[13] = ((p[5] + (p[6] << 1) + p[7] + 2) >> 2);
  pred[11] = pred[14] = ((p[6] + (p[7] << 1) + p[8] + 2) >> 2);
  pred[15] = ((p[7] + 3 * p[8] + 2) >> 2);
}

/*
Down-Right Prediction -> 0,5,10,15 = (I+2Q+A+2)/4 
                         1,6,11 = (Q+2A+B+2)/4  
                         2,7 = (A+2B+C+2)/4 
                         3 = (B+2C+D+2)/4
                         4,9,14 = (J+2I+Q+2)/4
                         8,13 = (K+2J+I+2)/4
                         12 = (L+2K+J+2)/4
*/
void intra4x4_downright(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;

  pred[0]  = pred[5]  = pred[10] = pred[15] = (((p[0] << 1) + p[1] + p[9] + 2) >> 2);
  pred[1]  = pred[6]  = pred[11] = ((p[0] + (p[1] << 1) + p[2] + 2) >> 2);
  pred[2]  = pred[7]  = ((p[1] + p[3] + (p[2] << 1) + 2) >> 2);
  pred[3]  = ((p[2] + (p[3] << 1) + p[4] + 2) >> 2);
  pred[4]  = pred[9]  = pred[14] = ((p[0] + (p[9] << 1) + p[10] + 2) >> 2);
  pred[8]  = pred[13] = ((p[9] + (p[10] << 1) + p[11] + 2) >> 2); 
  pred[12] = ((p[10] + (p[11] << 1) + p[12] + 2) >> 2); 
}


/*
Vertical-Left Prediction -> 0 = (A+B+1)/2 
                            1,8 = (B+C+1)/2  
                            2,9 = (C+D+1)/2
                            3,10 = (D+E+1)/2
                            4 = (A+2B+C+2)/4
                            5,12 = (B+2C+D+2)/4
                            6,13 = (C+2D+E+2)/4
                            7,14 = (D+2E+F+2)/4
                            11 = (E+F+1)/2
                            15 = (E+2F+G+2)/4
*/
void intra4x4_verticalleft(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
 
  pred[0]  = ((p[1] + p[2] + 1) >> 1);
  pred[1]  = pred[8]  = ((p[2] + p[3] + 1) >> 1);
  pred[2]  = pred[9]  = ((p[3] + p[4] + 1) >> 1);
  pred[3]  = pred[10] = ((p[4] + p[5] + 1) >> 1);
  pred[4]  = ((p[1] + (p[2] << 1) + p[3] + 2) >> 2);
  pred[5]  = pred[12] = ((p[2] + (p[3] << 1) + p[4] + 2) >> 2);
  pred[6]  = pred[13] = ((p[3] + (p[4] << 1) + p[5] + 2) >> 2);
  pred[7]  = pred[14] = ((p[4] + (p[5] << 1) + p[6] + 2) >> 2);
  pred[11] = ((p[5] + p[6] + 1) >> 1);
  pred[15] = ((p[5] + (p[6] << 1) + p[7] + 2) >> 2);
}


/*
Vertical-Right Prediction -> 0,9 = (Q+A+1)/2 
                             1,10 = (A+B+1)/2  
                             2,11 = (B+C+1)/2
                             3 = (C+D+1)/2
                             4,13 = (I+2Q+A+2)/4
                             5,14 = (Q+2A+B+2)/4
                             6,15 = (A+2B+C+2)/4
                             7 = (B+2C+D+2)/4
                             8 = (Q+2I+J+2)/4
                             12 = (I+2J+K+2)/4
*/
void intra4x4_verticalright(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;

  pred[0]  = pred[9]  = ((p[0] + p[1] + 1) >> 1);
  pred[1]  = pred[10] = ((p[1] + p[2] + 1) >> 1);
  pred[2]  = pred[11] = ((p[2] + p[3] + 1) >> 1);
  pred[3]  = ((p[3] + p[4] + 1) >> 1); 
  pred[4]  = pred[13] = (((p[0] << 1) + p[1] + p[9] + 2) >> 2);
  pred[5]  = pred[14] = ((p[0] + (p[1] << 1) + p[2] + 2) >> 2);
  pred[6]  = pred[15] = ((p[1] + (p[2] << 1) + p[3] + 2) >> 2);
  pred[7]  = ((p[2] + (p[3] << 1) + p[4] + 2) >> 2);
  pred[8]  = ((p[0] + (p[9] << 1) + p[10] + 2) >> 2); 
  pred[12] = ((p[9] + (p[10] << 1) + p[11] + 2) >> 2);
}


/*
Horizontal-Down Prediction -> 0,6 = (Q+I+1)/2 
                              1,7 = (I+2Q+A)/4  
                              2 = (Q+2A+B+2)/4
                              3 = (A+2B+C+2)/4
                              4,10 = (I+J+1)/2
                              5,11 = (Q+2I+J+2)/4
                              8,14 = (J+K+1)/2
                              9,15 = (I+2J+K+2)/4
                              12 = (K+L+1)/2
                              13 = (J+2K+L+2)/4
*/
void intra4x4_horizontaldown(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;

  pred[0]  = pred[6]  = ((p[0] + p[9] + 1) >> 1);
  pred[1]  = pred[7]  = (((p[0] << 1) + p[1] + p[9] +  + 2) >> 2);
  pred[2]  = ((p[0] + (p[1] << 1) + p[2] + 2) >> 2);
  pred[3]  = ((p[1] + (p[2] << 1) + p[3] + 2) >> 2);
  pred[4]  = pred[10] = ((p[9] + p[10] + 1) >> 1);
  pred[5]  = pred[11] = ((p[0] + (p[9] << 1) + p[10] + 2) >> 2);
  pred[8]  = pred[14] = ((p[10] + p[11] + 1) >> 1);
  pred[9]  = pred[15] = ((p[9] + (p[10] << 1) + p[11] + 2) >> 2);
  pred[12] = ((p[11] + p[12] + 1) >> 1);
  pred[13] = ((p[10] + (p[11] << 1) + p[12] + 2) >> 2);
}


/*
Horizontal-Up Prediction -> 0 = (I+J+1)/2 
                            1 = (I+2J+K+2)/4  
                            2,4 = (J+K+1)/2
                            3,5 = (J+2K+L+2)/4
                            6,8 = (K+L+1)/2
                            7,9 = (K+3L+2)/4
                            10,11,12,13,14,15 = L
*/
void intra4x4_horizontalup(CopyBlock4x4& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;

  pred[0]  = ((p[9] + p[10] + 1) >> 1);
  pred[1]  = ((p[9] + (p[10] << 1) + p[11] + 2) >> 2);
  pred[2]  = pred[4]  = ((p[10] + p[11] + 1) >> 1);
  pred[3]  = pred[5]  = ((p[10] + (p[11] << 1) + p[12] + 2) >> 2);
  pred[6]  = pred[8] = ((p[11] + p[12] + 1) >> 1);
  pred[7]  = pred[9] = ((p[11] + (3 * p[12]) + 2) >> 2);
  pred[10] = pred[11] = pred[12] = pred[13] = pred[14] = pred[15] = p[12];
}


/* Get intra4x4 predictors from neighbors
 * [0]: downmost and rightmost pixel of up-left (ul)-> Q
 * [1..4]: downmost row of up (u) -> A,B,C,D 
 * [5..8]: downmost row of up-right (ur) -> E,F,G,H
 * [9..12]: rightmost column of left (l) -> I,J,K,L
 * 
 * There are 4 blocks that are possible neighbours (a,b,c,d)
 * 
 */

Predictor get_intra4x4_predictor(std::experimental::optional<Block4x4> ul, std::experimental::optional<Block4x4> u,
                                 std::experimental::optional<Block4x4> ur, std::experimental::optional<Block4x4> l)
{
  // 4x4 block predictor (ul, 4xU, 4xUR, 4xL)
  Predictor predictor(4);
  std::vector<int>& p = predictor.pred_pel;
  
  // Check whether neighbors are available, check image get_predictors_neighbours

  // If up predictor is avaliable copy bottom row from upper block (b-block) (12-15) to A,B,C,D
  if (u) 
  {
    Block4x4& tmp = *u;
    std::copy_n(tmp.begin()+4*3, 4, p.begin()+1);
    predictor.up_available = true;
  }
  else
  {
    std::fill_n(p.begin()+1, 4, 128);   // if not avaliable assumes A,B,C,D at 128
  }


  // If up-right predictor is avaliable copy bottom row from upper-right block (c-block) (12-15) to E,F,G,H
  if (ur) 
  {
    Block4x4& tmp = *ur;
    std::copy_n(tmp.begin()+4*3, 4, p.begin()+5);
    predictor.up_right_available = true;
  }
  else      // If predictor not avaliable assumes E,F,G,H as D
  {
    std::fill_n(p.begin()+5, 4, p[4]);
  }

  // If left predictor is avaliable copy right row from left block (a-block) (3,7,11,15) to I,J,K,L
  if (l) 
  {
    Block4x4& tmp = *l;
    for (int i = 0; i < 4; i++) 
    {
      p[9+i] = tmp[i*4+3];
    }
    predictor.left_available = true;
  }
  else 
  {
    std::fill_n(p.begin()+9, 4, 128);   // If predictor not avaliable assumes I,J,K,L as 128
  }

  // If both up and left predictors are avaliable -> up-left predictor is avaliable, copies bit 15 (bottom-right) to Q predictor
  if (predictor.up_available && predictor.left_available) 
  {
    Block4x4& tmp = *ul;
    p[0] = tmp[15];
    predictor.all_available = true;
  }
  else 
  {
    p[0] = 128;  // If predictor not avaliable assumes Q to 128
  }

  return predictor;
}


////////////////////////////////////////// 16 x 16 MODES /////////////////////////////////////////////////////////////

/* Input 16x16 block and its neighbors
 * Do Intra 16x16 prediction which has 4 modes
 * Overwrite residual on input block
 * Return the least cost mode
 */

std::tuple<int, Intra16x16Mode> intra16x16(Block16x16& block, std::experimental::optional<std::reference_wrapper<Block16x16>> ul,
                                                              std::experimental::optional<std::reference_wrapper<Block16x16>> u,
                                                              std::experimental::optional<std::reference_wrapper<Block16x16>> l) {

  ofstream myfile ("txt/16x16_Y_predictors.txt", ios::app);
  static int predictor_cnt=0;

  // Get predictors
  Predictor predictor = get_intra16x16_predictor(ul, u, l);

  // Print predictors to '16x16predictors.txt'
  myfile << "MB " << predictor_cnt << " ->";
  if(predictor.up_available)
    myfile << "Up_aval "; 
  if (predictor.left_available)
    myfile << "Left_aval ";
  if (predictor.up_right_available)
    myfile << "Up_right_aval ";
  if (predictor.all_available)
    myfile << "All_aval ";
  for (int i = 0; i < 33; i++)
  {
    myfile << predictor.pred_pel[i] << ' ';
  }
  myfile << endl;
  predictor_cnt++;

  int mode;
  Intra16x16Mode best_mode;
  Block16x16 pred, best_pred, residual, best_residual;
  int min_sad = (1 << 15), sad;  // worst SAD is 32768 -> 16*16 pixels = 256; worst prediction = 128-0 ; 256*128 = 32768

  // Run all 16x16 pred modes to get least residual
  for (mode = 0; mode < 4; mode++) {

    // cast is to convert from int to enum
    // Checks for the predictors needed to perform each prediction mode. Continue jumps iteration
    if ((!predictor.up_available   && (Intra16x16Mode::VERTICAL   == static_cast<Intra16x16Mode>(mode))) ||
        (!predictor.left_available && (Intra16x16Mode::HORIZONTAL == static_cast<Intra16x16Mode>(mode))) ||
        (!predictor.all_available  && (Intra16x16Mode::PLANE      == static_cast<Intra16x16Mode>(mode)))) {
      continue;
    }

    // Run prediction, save in pred
    get_intra16x16(pred, predictor, static_cast<Intra16x16Mode>(mode));

    // Computes SAD, save best prediction and residual
    sad = SAD(block.begin(), block.end(), pred.begin(), residual.begin());
    if (sad < min_sad) {
      min_sad = sad;
      best_mode = static_cast<Intra16x16Mode>(mode);
      std::copy(residual.begin(), residual.end(), best_residual.begin());   // save best residual block
      std::copy(pred.begin(), pred.end(), best_pred.begin());   // save best prediction
    }
  }

  std::copy(best_pred.begin(), best_pred.end(), block.begin());   // Overwrite input block with best predicted
  // std::copy(best_residual.begin(), best_residual.end(), block.begin());   // Overwrite input block with best residual


  return std::make_tuple(min_sad, best_mode);
}


// Input 16x16 predictors and mode 
void get_intra16x16(Block16x16& pred, const Predictor& p, const Intra16x16Mode mode) {
  switch (mode) {
    case Intra16x16Mode::VERTICAL:
      intra16x16_vertical(pred, p);
      break;
    case Intra16x16Mode::HORIZONTAL:
      intra16x16_horizontal(pred, p);
      break;
    case Intra16x16Mode::DC:
      intra16x16_dc(pred, p);
      break;
    case Intra16x16Mode::PLANE:
      intra16x16_plane(pred, p);
      break;
  }
}

/* Get intra16x16 predictors from neighbors
 * [0]: downmost and rightmost pixel of ul
 * [1..16]: downmost row of u
 * [17..32]: rightmost column of l
 */
Predictor get_intra16x16_predictor(
  std::experimental::optional<std::reference_wrapper<Block16x16>> ul,
  std::experimental::optional<std::reference_wrapper<Block16x16>> u,
  std::experimental::optional<std::reference_wrapper<Block16x16>> l) {

  Predictor predictor(16);
  std::vector<int>& p = predictor.pred_pel;
  // Check whether neighbors are available
  if (u) {
    Block16x16& tmp = *u;
    std::copy_n(tmp.begin()+16*15, 16, p.begin()+1);
    predictor.up_available = true;
  }
  else {
    std::fill_n(p.begin()+1, 16, 128);
  }

  if (l) {
    Block16x16& tmp = *l;
    for (int i = 0; i < 16; i++) {
      p[17+i] = tmp[i*16+15];
    }
    predictor.left_available = true;
  }
  else {
    std::fill_n(p.begin()+17, 16, 128);
  }

  if (predictor.up_available && predictor.left_available) {
    Block16x16& tmp = *ul;
    p[0] = tmp.back();
    predictor.all_available = true;
  }
  else {
    p[0] = 128;
  }

  return predictor;
}

/*
      H

v   16X16 block

H and V rows are the predictors 
*/


// Vertical prediction -> All pixels are equal to H

void intra16x16_vertical(Block16x16& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel; // predictor elements
  int i;
  for (i = 0; i < 16; i++) {
    // first pixel is UL, then the U pixels
    std::copy_n(p.begin()+1, 16, pred.begin()+i*16);
  }
}

// Horizontal prediction -> All pixels are equal to V

void intra16x16_horizontal(Block16x16& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int i, j;
  for (i = 0; i < 16; i++) {
    for (j = 0; j < 16; j++) {
      pred[i*16+j] = p[17+i];
    }
  }
}


// DC prediction -> (V+H+16)/32

void intra16x16_dc(Block16x16& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int s1 = 0, s2 = 0, s = 0;
  int i;

  for (i = 1; i < 17; i++) {
    s1 += p[i];   // accumulates upper pixels
  }

  for (i = 17; i < 33; i++) {
    s2 += p[i];   // accumulates left pixels
  }

  // predictor availability should be checked first !!!!
  if (predictor.up_available && predictor.left_available) {
    s = s1 + s2;
  }
  else if (!predictor.up_available && predictor.left_available) {
    s = 2 * s2;
  }
  else if (predictor.up_available && !predictor.left_available) {
    s = 2 * s1;
  }

  s += 16;
  s >>= 5;

  if (!predictor.up_available && !predictor.left_available) {
    s = 128;
  }

  pred.fill(s);
}

// Plane prediction

void intra16x16_plane(Block16x16& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int H = 0, V = 0;
  int a, b, c;
  int i, j;

  for (i = 1; i < 8; i++) {
    H += i * (p[8+i] - p[8-i]);
    V += i * (p[24+i] - p[24-i]);
  }

  H += 8 * (p[16] - p[0]);
  V += 8 * (p[32] - p[0]);

  a = 16 * (p[16] + p[32]);
  b = (5 * H + 32) >> 6;
  c = (5 * V + 32) >> 6;

  for (i = 0; i < 16; i++) {
    for (j = 0; j < 16; j++) {
      pred[i*16+j] = clip((a + b * (j-7) + c * (i-7) + 16) >> 5, 0, 255);
    }
  }
}


////////////////////////////////////////// 8 x 8 MODES /////////////////////////////////////////////////////////////

/*
  8x8 prediction is similar to 16x16 but with smaller blocks
*/


/* Input 8x8 chroma block and its neighbors
 * do intra8x8 prediction which has 4 modes
 * overwrite residual on input block
 * return the least cost mode
 */
std::tuple<int, IntraChromaMode> intra8x8_chroma(Block8x8& cr_block, std::experimental::optional<std::reference_wrapper<Block8x8>> cr_ul,
  std::experimental::optional<std::reference_wrapper<Block8x8>> cr_u, std::experimental::optional<std::reference_wrapper<Block8x8>> cr_l,
  Block8x8& cb_block, std::experimental::optional<std::reference_wrapper<Block8x8>> cb_ul,
  std::experimental::optional<std::reference_wrapper<Block8x8>> cb_u, std::experimental::optional<std::reference_wrapper<Block8x8>> cb_l) {

  ofstream Cb_pred_file ("txt/8x8_Cb_predictors.txt", ios::app);
  ofstream Cr_pred_file ("txt/8x8_Cr_predictors.txt", ios::app);
  static int predictor_cnt=0;

  // Get Cr, Cb predictors
  Predictor cr_predictor = get_intra8x8_chroma_predictor(cr_ul, cr_u, cr_l);
  Predictor cb_predictor = get_intra8x8_chroma_predictor(cb_ul, cb_u, cb_l);

  // Print Cb predictors to '8x8_Cb_predictors.txt'
  Cb_pred_file << "MB " << predictor_cnt << " -> ";
  for (int i = 0; i < 17; i++)
  {
    Cb_pred_file << cb_predictor.pred_pel[i] << ' ';
  }
  Cb_pred_file << endl;

  // Print Cr predictors to '8x8_Cr_predictors.txt'
  Cr_pred_file << "MB " << predictor_cnt << " -> ";
  for (int i = 0; i < 17; i++)
  {
    Cr_pred_file << cr_predictor.pred_pel[i] << ' ';
  }
  Cr_pred_file << endl;

  predictor_cnt++;

  int mode;
  IntraChromaMode best_mode;
  Block8x8 cr_pred, cb_pred, cr_residual, cb_residual;
  Block8x8 cr_best_pred, cb_best_pred, cr_best_residual, cb_best_residual;
  int min_sad = (1 << 15), cr_sad, cb_sad, sad;
  // Run all modes to get least residual
  for (mode = 0; mode < 4; mode++) {
    if ((!cr_predictor.up_available   && (IntraChromaMode::VERTICAL   == static_cast<IntraChromaMode>(mode))) ||
        (!cr_predictor.left_available && (IntraChromaMode::HORIZONTAL == static_cast<IntraChromaMode>(mode))) ||
        (!cr_predictor.all_available  && (IntraChromaMode::PLANE      == static_cast<IntraChromaMode>(mode)))) {
      continue;
    }

    get_intra8x8_chroma(cr_pred, cr_predictor, static_cast<IntraChromaMode>(mode));
    get_intra8x8_chroma(cb_pred, cb_predictor, static_cast<IntraChromaMode>(mode));

    // According to the standard, prediction mode must be the same for both Cb and Cr blocks
    // NOTE: after testing, performance should be improved copying the residual to the pred block
    cr_sad = SAD(cr_block.begin(), cr_block.end(), cr_pred.begin(), cr_residual.begin()); // <- //
    cb_sad = SAD(cb_block.begin(), cb_block.end(), cb_pred.begin(), cb_residual.begin()); // <- //
    sad = cr_sad + cb_sad;
    if (sad < min_sad) {
      min_sad = sad;
      best_mode = static_cast<IntraChromaMode>(mode);
      std::copy(cr_pred.begin(), cr_pred.end(), cr_best_pred.begin());  // save best predicted Cr
      std::copy(cb_pred.begin(), cb_pred.end(), cb_best_pred.begin());  // save best predicted Cb
      std::copy(cr_residual.begin(), cr_residual.end(), cr_best_residual.begin());    // save best Cr residual
      std::copy(cb_residual.begin(), cb_residual.end(), cb_best_residual.begin());    // save best Cb residual
    }
  }
  
  // copy best pred to original block (TESTING !!!)
  std::copy(cr_best_pred.begin(), cr_best_pred.end(), cr_block.begin());
  std::copy(cb_best_pred.begin(), cb_best_pred.end(), cb_block.begin());

  return std::make_tuple(min_sad, best_mode);
}


/* Get intra8x8 chroma predictors from neighbors
 * [0]: downmost and rightmost pixel of ul
 * [1..8]: downmost row of u
 * [9..16]: rightmost column of l
 */
Predictor get_intra8x8_chroma_predictor(std::experimental::optional<std::reference_wrapper<Block8x8>> ul,
                                        std::experimental::optional<std::reference_wrapper<Block8x8>> u,
                                        std::experimental::optional<std::reference_wrapper<Block8x8>> l) {

  Predictor predictor(8);
  std::vector<int>& p = predictor.pred_pel;
  
  // Check whether neighbors are available
  if (u) {
    Block8x8& tmp = *u;
    std::copy_n(tmp.begin()+8*7, 8, p.begin()+1);
    predictor.up_available = true;
  }
  else {
    std::fill_n(p.begin()+1, 8, 128);
  }

  if (l) {
    Block8x8& tmp = *l;
    for (int i = 0; i < 8; i++) {
      p[9+i] = tmp[i*8+7];
    }
    predictor.left_available = true;
  }
  else {
    std::fill_n(p.begin()+9, 8, 128);
  }

  if (predictor.up_available && predictor.left_available) {
    Block8x8& tmp = *ul;
    p[0] = tmp.back();
    predictor.all_available = true;
  }
  else {
    p[0] = 128;
  }

  return predictor;
}



// Input predictors and mode 

void get_intra8x8_chroma(Block8x8& pred, const Predictor& p, const IntraChromaMode mode) {
  switch (mode) {
    case IntraChromaMode::DC:
      intra8x8_chroma_dc(pred, p);
      break;
    case IntraChromaMode::HORIZONTAL:
      intra8x8_chroma_horizontal(pred, p);
      break;
    case IntraChromaMode::VERTICAL:
      intra8x8_chroma_vertical(pred, p);
      break;
    case IntraChromaMode::PLANE:
      intra8x8_chroma_plane(pred, p);
      break;
  }
}


// DC Prediction
void intra8x8_chroma_dc(Block8x8& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int s1 = 0, s2 = 0, s3 = 0, s4 = 0;
  int s_upper_left = 0, s_upper_right = 0, s_down_left = 0, s_down_right = 0;
  int i, j;

  // summation of predictors
  // s1: [1..4], s2: [5..8]
  // s3: [9..12], s4: [13..16]
  for (i = 0; i < 4; i++) {
    s1 += p[i+1];
    s2 += p[i+5];
    s3 += p[i+9];
    s4 += p[i+13];
  }

  if (predictor.up_available && predictor.left_available) {
    s_upper_left = s1 + s3;
    s_upper_right = s2 + s3;
    s_down_left = s1 + s4;
    s_down_right = s2 + s4;
  }
  else if (!predictor.up_available && predictor.left_available) {
    s_upper_left = s_upper_right = 2 * s3;
    s_down_left = s_down_right = 2 * s4;
  }
  else if (predictor.up_available && !predictor.left_available) {
    s_upper_left = s_down_left = 2 * s1;
    s_upper_right = s_down_right = 2 * s2;
  }

  s_upper_left = (s_upper_left + 4) >> 3;
  s_upper_right = (s_upper_right + 4) >> 3;
  s_down_left = (s_down_left + 4) >> 3;
  s_down_right = (s_down_right + 4) >> 3;

  if (!predictor.up_available && !predictor.left_available) {
    s_upper_left = s_upper_right = s_down_left = s_down_right  = 128;
  }

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      pred[i*8+j] = s_upper_left;
      pred[i*8+(j+4)] = s_upper_right;
      pred[(i+4)*8+j] = s_down_left;
      pred[(i+4)*8+(j+4)] = s_down_right;
    }
  }
}

// Horizontal prediction
void intra8x8_chroma_horizontal(Block8x8& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int i, j;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      pred[i*8+j] = p[9+i];
    }
  }
}

// Vertical prediction
void intra8x8_chroma_vertical(Block8x8& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int i;
  for (i = 0; i < 8; i++) {
    std::copy_n(p.begin()+1, 8, pred.begin()+i*8);
  }
}

// Plane prediciton
void intra8x8_chroma_plane(Block8x8& pred, const Predictor& predictor) {
  const std::vector<int>& p = predictor.pred_pel;
  int H = 0, V = 0;
  int a, b, c;
  int i, j;

  for (i = 1; i < 4; i++) {
    H += i * (p[4+i] - p[4-i]);
    V += i * (p[12+i] - p[12-i]);
  }

  H += 4 * (p[8] - p[0]);
  V += 4 * (p[16] - p[0]);

  a = 16 * (p[8] + p[16]);
  b = (17 * H + 16) >> 5;
  c = (17 * V + 16) >> 5;

  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      pred[i*8+j] = clip((a + b * (j-3) + c * (i-3) + 16) >> 5, 0, 255);
    }
  }
}
