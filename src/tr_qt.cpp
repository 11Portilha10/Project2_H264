#include "tr_qt.h"

/**
 * Transform and Quantize block overview
 * 
 * 
 * The basic transform, ‘core transform’, is a 4×4 or 8×8 integer transform, a scaled approximation to the Discrete Cosine Transform, DCT
 * 
 * In the luma default process, each 4 × 4 block within the 16 × 16 luma region of the macroblock is transformed (Cf 4), scaled and quantized (Mf 4). 
 * The coefficient blocks are coded and transmitted in the order shown (see transform_luma_default.png)
 * 
 * 
 * If the macroblock is predicted using 16×16 Intra Prediction, a second transform (4 × 4 Hadamard transform) is applied to 
 * the 4 lowest or ‘DC’ frequency coefficients of the first transform 4x4 blocks. (see transform_luma16x16_mode.png)
 * 
 * 
 * For chroma, each 4×4 block of Cb or Cr samples is transformed (Cf4). 
 * The four DC coefficients of each block are further transformed with a 2 × 2 Hadamard or DCT transform. The two DC blocks, labelled 0 and 1, followed by
 * the AC blocks 2 to 9 are scaled, quantized and transmitted. (see transform_chroma.png)
 * 
 */


/////////////////////////// TRANSFORM FUNCTIONS ///////////////////////////////////////7


/* Quantized discrete cosine transformation
 *
 * The interface of forward QDCT (for 16x16 and 8x8 blocks), apply on each 4x4 block
*/
template <typename T>
inline void forward_qdct(T& block, const int BLOCK_SIZE, const int QP) {

  // Source 4x4 block (mat_x) and target 4x4 block (mat_z)
  int mat_x[4][4], mat_z[4][4];

  // Apply 4x4 core transform 16 times on 16x16 block -> Entire 16x16 is transformed with core transformation
  for (int i = 0; i < BLOCK_SIZE*BLOCK_SIZE; i += BLOCK_SIZE*4) {
    for (int j = 0; j < BLOCK_SIZE; j += 4){
      // Copy into 4x4 source matrix
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++)
          mat_x[y][x] = block[i+j+y*BLOCK_SIZE+x];
      }

      // Apply 4x4 core transform (source x to target z)
      forward_dct4x4(mat_x, mat_z);

      // Write back from 4x4 matrix (after doing transform)
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++)
          block[i+j+y*BLOCK_SIZE+x] = mat_z[y][x];
      }
    }
  }


  int mat16[4][4], mat8[2][2];
  
  // If is a 16x16 block (Luma). This 16x16 block can be decomposed into 16 4x4 matrix 
  if (BLOCK_SIZE == 16) {
    // Copies the first pixel [0] from each 4x4 matrix inside 16x16 to an auxiliar 4x4 matrix
    // Block 4x4 copied:
    /*  0    4    8    12
    *   64   68   72   76
    *   128  132  136  140
    *   192  196  200  204
    */
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        mat16[i][j] = block[i*4*BLOCK_SIZE + j*4];
      }
    }
    
    // Applies hadamard 4x4 transform 
    forward_hadamard4x4(mat16, mat_x);

    // Applies 4x4 quantization (DC)
    forward_DC_quantize4x4(mat_x, mat16, QP);
  }

  // BLOCK_SIZE = 8. If is a 8x8 block (Chroma). This 8x8 block can be decomposed into 4 4x4 matrix 
  else { 
    
    int mat_p[2][2];

    // Copies the first pixel [0] from each 4x4 matrix inside 16x16 to an auxiliar 2x2 matrix
    // Block 4x4 copied:
    /*  0   4  
    *   32  36 
    */
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        mat8[i][j] = block[i*4*BLOCK_SIZE + j*4];
      }
    }

    // Applies hadamard 2x2 transform
    forward_hadamard2x2(mat8, mat_p);

    // Applies 2x2 quantization (DC)
    forward_quantize2x2(mat_p, mat8, QP);
  }

  // Apply 4x4 (AC) quantization 16 times on 16x16 block
  for (int i = 0; i < BLOCK_SIZE*BLOCK_SIZE; i += BLOCK_SIZE*4) {
    for (int j = 0; j < BLOCK_SIZE; j += 4) {
      // Copy into 4x4 matrix
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++)
          mat_x[y][x] = block[i+j+y*BLOCK_SIZE+x];
      }

      // Apply 4x4 quantization (AC)
      forward_quantize4x4(mat_x, mat_z, QP);

      // Write back from 4x4 matrix (AC component)
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++)
          block[i+j+y*BLOCK_SIZE+x] = mat_z[y][x];
      }
    }
  }

  // Write back from 4x4 matrix (DC component) 
  if (BLOCK_SIZE == 16) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++)
        block[i*4*BLOCK_SIZE + j*4] = mat16[i][j];
    }
  }

  // Write back from 2x2 matrix (DC component)
  else { // BLOCK_SIZE = 8
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++)
        block[i*4*BLOCK_SIZE + j*4] = mat8[i][j];
    }
  }
}


/* Quantized discrete cosine transformation
 *
 * The interface of forward QDCT apply on each 4x4 block
*/
inline void forward_qdct4x4(Block4x4 block, const int QP){

  // source 4x4 block, target 4x4 block
  int mat_x[4][4], mat_z[4][4];

  // Copy into 4x4 matrix
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++)
      mat_x[y][x] = block[y*4+x];
  }

  // Apply 4x4 core transform
  forward_dct4x4(mat_x, mat_z);

  // Aply 4x4 quantization
  forward_quantize4x4(mat_z, mat_x, QP);

  // Write back from 4x4 matrix
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++)
      block[y*4+x] = mat_x[y][x];
  }
}


/* Core transformation (4x4)
 *
 * given the residual matrix: R, the core matrix: W, is
 *   W = Cf x R x Cf^T
 */
void forward_dct4x4(const int mat_x[][4], int mat_z[][4]) {
  int mat_temp[4][4];
  int p0, p1, p2, p3, t0, t1, t2, t3;

  // Horizontal
  for (int i = 0; i < 4; i++) {
    p0 = mat_x[i][0];
    p1 = mat_x[i][1];
    p2 = mat_x[i][2];
    p3 = mat_x[i][3];

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    mat_temp[i][0] = t0 + t1;
    mat_temp[i][1] = (t3 << 1) + t2;
    mat_temp[i][2] = t0 - t1;
    mat_temp[i][3] = t3 - (t2 << 1);
  }

  // Vertical
  for (int i = 0; i < 4; i++) {
    p0 = mat_temp[0][i];
    p1 = mat_temp[1][i];
    p2 = mat_temp[2][i];
    p3 = mat_temp[3][i];

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    mat_z[0][i] = t0 + t1;
    mat_z[1][i] = t2 + (t3 << 1);
    mat_z[2][i] = t0 - t1;
    mat_z[3][i] = t3 - (t2 << 1);
  }
}

// Hadamard transformation on 4x4 block
void forward_hadamard4x4(const int mat_x[][4], int mat_z[][4]) {
  int mat_temp[4][4];
  int p0, p1, p2, p3, t0, t1, t2, t3;

  // Horizontal
  for (int i = 0; i < 4; i++) {
    p0 = mat_x[i][0];
    p1 = mat_x[i][1];
    p2 = mat_x[i][2];
    p3 = mat_x[i][3];

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    mat_temp[i][0] = t0 + t1;
    mat_temp[i][1] = t3 + t2;
    mat_temp[i][2] = t0 - t1;
    mat_temp[i][3] = t3 - t2;
  }

  // Vertical
  for (int i = 0; i < 4; i++) {
    p0 = mat_temp[0][i];
    p1 = mat_temp[1][i];
    p2 = mat_temp[2][i];
    p3 = mat_temp[3][i];

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    mat_z[0][i] = (t0 + t1 + 1) >> 1;
    mat_z[1][i] = (t2 + t3 + 1) >> 1;
    mat_z[2][i] = (t0 - t1 + 1) >> 1;
    mat_z[3][i] = (t3 - t2 + 1) >> 1;
  }
}

// Hadamard transformation on 2x2 block
void forward_hadamard2x2(const int mat_x[][2], int mat_z[][2]) {
  int p0, p1, p2, p3;

  p0 = mat_x[0][0] + mat_x[0][1];
  p1 = mat_x[0][0] - mat_x[0][1];
  p2 = mat_x[1][0] + mat_x[1][1];
  p3 = mat_x[1][0] - mat_x[1][1];

  mat_z[0][0] = p0 + p2;
  mat_z[0][1] = p1 + p3;
  mat_z[1][0] = p0 - p2;
  mat_z[1][1] = p1 - p3;
}


// QDCT -> Quantized Discrete Cosine Transform

// Performs 16x16 Luma QDCT 
void qdct_luma16x16_intra(Block16x16& block){
  forward_qdct(block, 16, LUMA_QP);
}


// Performs 8x8 Chroma QDCT
void qdct_chroma8x8_intra(Block8x8& block){
  forward_qdct(block, 8, CHROMA_QP);
}


// Performs 4x4 Luma QDCT -> não passa o bloco por referência?
void qdct_luma4x4_intra(Block4x4 block){
  forward_qdct4x4(block, LUMA_QP);
}


//////////////////////////////// QUANTIZATION FUNCTIONS ////////////////////////////////

/* Quantization
 *
 * By formula:
 *   (0, 0),(2, 0),(0, 2),(2, 2): mf = mat_MF[QP % 6][0]
 *   (1, 1),(3, 1),(1, 3),(3, 3): mf = mat_MF[QP % 6][1]
 *   other positions:             mf = mat_MF[QP % 6][2]
 */
void forward_quantize4x4(const int mat_x[][4], int mat_z[][4], const int QP){
  int qbits = 15 + floor(QP / 6);
  int f = (int)(pow(2.0, qbits) / 3.0);
  int k;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if ((i == 0 || i == 2) && (j == 0 || j == 2))
        k = 0;
      else if ((i == 1 || i == 3) && (j == 1 || j == 3))
        k = 1;
      else
        k = 2;

      mat_z[i][j] = (abs(mat_x[i][j]) * mat_MF[QP % 6][k] + f) >> qbits;
      if (mat_x[i][j] < 0)
        mat_z[i][j] = -mat_z[i][j];
    }
  }
}


// DC 4x4 Quantization
void forward_DC_quantize4x4(const int mat_x[][4], int mat_z[][4], const int QP){
  int qbits = 15 + floor(QP / 6);
  int f = (int)(pow(2.0, qbits) / 3.0);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mat_z[i][j] = (abs(mat_x[i][j]) * mat_MF[QP % 6][0] + 2 * f) >> (qbits + 1);
      if (mat_x[i][j] < 0)
        mat_z[i][j] = -mat_z[i][j];
    }
  }
}

// 2x2 Quantization
void forward_quantize2x2(const int mat_x[][2], int mat_z[][2], const int QP) {
  int qbits = 15 + floor(QP / 6);
  int f = (int)(pow(2.0, qbits) / 3.0);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      mat_z[i][j] = (abs(mat_x[i][j]) * mat_MF[QP % 6][0] + 2 * f) >> (qbits + 1);
      if (mat_x[i][j] < 0)
        mat_z[i][j] = -mat_z[i][j];
    }
  }
}