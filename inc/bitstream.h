#ifndef BITSTREAM_H_
#define BITSTREAM_H_

#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
#include <bitset>

class Bitstream {
public:
  int nb_bits;    // total number of bits in the stream
  std::vector<std::uint8_t> buffer;   // store bits as unsigned chars in a vector

  Bitstream();
  Bitstream(const bool&);
  Bitstream(std::uint8_t[], int);
  Bitstream(const Bitstream&);
  Bitstream(const std::string&);
  Bitstream(const std::uint8_t, int);
  Bitstream(const unsigned int, int);

  Bitstream operator+(const Bitstream&);
  Bitstream& operator+=(const Bitstream&);

  bool byte_align();
  Bitstream rbsp_trailing_bits();
  Bitstream rbsp_to_ebsp();

  // for testing
  std::string to_string();
};

#endif // BITSTREAM

