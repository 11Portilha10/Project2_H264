#ifndef VLC_H
#define VLC_H

#include <cmath>
#include <bitset>
#include <string>
#include <cstdint>

#include "block.h"
#include "bitstream.h"

const int me[] = {
	3 , 29, 30, 17, 31,
	18, 37, 8 , 32, 38, 
	19, 9 , 20, 10, 11, 
	2 , 16, 33, 34, 21, 
	35, 22, 39, 4 , 36, 
	40, 23, 5 , 24, 6 , 
	7 , 1 , 41, 42, 43, 
	25, 44, 26, 46, 12, 
	45, 47, 27, 13, 28, 
	14, 15, 0
};

typedef std::string (*level_VLC_encoder)(int);

// Level VLC tables
std::string level_VLC_0(int level_code);
std::string level_VLC_1(int level_code);
std::string level_VLC_2(int level_code);
std::string level_VLC_3(int level_code);
std::string level_VLC_4(int level_code);
std::string level_VLC_5(int level_code);
std::string level_VLC_6(int level_code);

/**
 * @brief Unsigned Exponential Golomb Coding
 * 
 */
Bitstream uegc(const unsigned int);

/**
 * @brief Signed Exponential Golomb Coding
 * 
 */
Bitstream segc(const int);

/**
 * @brief       Performs 4x4 CAVLC encoding
 * 
 * @param block Transform coefficients 4x4 input block
 * @param nC    Number of non-zero coefficients in neighbouring blocks
 * @param maxNumCoeff 15 or 16?
 * 
 * @return  Bitstream object initialized with the final string
 */
std::pair<Bitstream, int> cavlc_block2x2(Block2x2, const int, const int);

/**
 * @brief       Performs 2x2 CAVLC encoding
 * 
 * @param block Transform coefficients 2x2 input block
 * @param nC    Number of non-zero coefficients in neighbouring blocks (-1 for chroma)
 * @param maxNumCoeff 4 for 2x2
 * 
 * @return  Bitstream object initialized with the final string
 * 
 * @note  The procedure is the same as in the 4x4 CAVLC, except for the indexes.
 *        Comments in 4x4 CAVLC also apply here
 */
std::pair<Bitstream, int> cavlc_block4x4(Block4x4, const int, const int);

#endif