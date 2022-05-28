#include <iostream>
#include "bitstream.h"

Bitstream::Bitstream(): nb_bits(0) {}

/**
 * @brief Stores a bool value into as an uint8_t value (in the MSb).
 *        nb_bits is set to 1.
 * 
 * @param flag Bool value to be saved
 */
Bitstream::Bitstream(const bool& flag) {
  nb_bits = 1;
  if (flag) {
    buffer.push_back(static_cast<std::uint8_t>(1 << 7));
  }
  else {
    buffer.push_back(static_cast<std::uint8_t>(0 << 7));
  }
}

/**
 * @brief Stores an array of uint8_t values in the stream vector.
 *        
 * 
 * @param bits  Array of bytes to be stored
 * @param digit Number of bits to be saved
 * 
 * @note Does NOT consider the case where 'digit' is not multiple of 8
 */
Bitstream::Bitstream(std::uint8_t bits[], int digit) {
  nb_bits = digit;

  int nb_int = digit % 8 == 0 ? digit/8 : digit/8 + 1;
  for (int i = 0; i < nb_int; i++)
    buffer.push_back(bits[i]);
}

/**
 * @brief Construct a new Bitstream from other Bitstream
 * 
 * @param a The bitstream to be copied
 */
Bitstream::Bitstream(const Bitstream& a) {
  nb_bits = a.nb_bits;
  buffer.insert(buffer.end(), a.buffer.begin(), a.buffer.end());
}

/**
 * @brief Construct a variable sized bitstream from a string bitset
 * 
 * @param s The string to be converted to a Bitstream object
 */
Bitstream::Bitstream(const std::string& s) {
  nb_bits = s.size();
  int nb_int = nb_bits % 8 == 0 ? nb_bits/8: nb_bits/8 + 1;
  int trail_bits = nb_bits % 8;
  
  for (int i = 0; i < nb_int - 1; i++)
    buffer.push_back( (std::bitset<8>(s.substr(i*8, 8)).to_ulong()) );

  if (trail_bits != 0)
    buffer.push_back( (std::bitset<8>(s.substr((nb_int-1)*8, trail_bits)).to_ulong() << (8-trail_bits)) );
  else
    buffer.push_back( (std::bitset<8>(s.substr((nb_int-1)*8, 8)).to_ulong()) );
}

/**
 * @brief Construct a bitstream object from a single uint8_t
 * 
 * @param u The byte to construct the bitstream
 * @param digit The number of bits
 */
Bitstream::Bitstream(const std::uint8_t u, int digit) {
  nb_bits = digit;
  buffer.push_back(u << (8-digit));
}

/**
 * @brief Decompose a 32bit value into 4 uint8_t, and store in a bitstream object
 * 
 * @param cui   The input word
 * @param digit The number of representative bits in the word
 */
Bitstream::Bitstream(const unsigned int cui, int digit) {
  nb_bits = digit;
  unsigned int ui = cui << (32 - digit);
  if (digit >= 0)
    buffer.push_back((std::uint8_t)(ui >> 24));
  if (digit >= 8)
    buffer.push_back((std::uint8_t)(ui >> 16));
  if (digit >= 16)
    buffer.push_back((std::uint8_t)(ui >> 8));
  if (digit >= 24)
    buffer.push_back((std::uint8_t)(ui));
}

Bitstream& Bitstream::operator+=(const Bitstream& a) {
  int trail_bits = this->nb_bits % 8;

  if (trail_bits == 0) {
    this->buffer.insert(this->buffer.end(), a.buffer.begin(), a.buffer.end());
  }
  else {
    std::uint8_t tmp = this->buffer.back();
    this->buffer.pop_back();

    // clear tmp
    tmp &= (0b11111111 << (8 - trail_bits));

    int last_bits = a.nb_bits;
    auto itr = a.buffer.begin();

    while (last_bits >= 8) {
      tmp |= *itr >> trail_bits;
      this->buffer.push_back(tmp);
      tmp = *itr << (8 - trail_bits);
      last_bits -= 8;
      itr++;
    }

    tmp |= *itr >> trail_bits;
    this->buffer.push_back(tmp);

    if (last_bits > (8 - trail_bits)) {
      tmp = *itr << (8 - trail_bits);
      this->buffer.push_back(tmp);
    }
  }

  this->nb_bits += a.nb_bits;
  return *this;
}

Bitstream Bitstream::operator+(const Bitstream& a) {
  Bitstream c(*this);
  int trail_bits = c.nb_bits % 8;

  if (trail_bits == 0) {
    c.buffer.insert(c.buffer.end(), a.buffer.begin(), a.buffer.end());
  }
  else {
    std::uint8_t tmp = c.buffer.back();
    c.buffer.pop_back();

    // clear tmp
    tmp &= (0b11111111 << (8 - trail_bits));

    int last_bits = a.nb_bits;
    auto itr = a.buffer.begin();

    while (last_bits >= 8) {
      tmp |= *itr >> trail_bits;
      c.buffer.push_back(tmp);
      tmp = *itr << (8 - trail_bits);
      last_bits -= 8;
      itr++;
    }

    tmp |= *itr >> trail_bits;
    c.buffer.push_back(tmp);

    if (last_bits > (8 - trail_bits)) {
      tmp = *itr << (8 - trail_bits);
      c.buffer.push_back(tmp);
    }
  }

  c.nb_bits += a.nb_bits;
  return c;
}

/**
 * @brief 
 * 
 * @return true if the number of bits is multiple of 8
 * @return false if the number of bits is NOT multiple of 8
 */
bool Bitstream::byte_align() {
  return (nb_bits % 8 == 0) ? true : false;
}

/**
 * @brief This function adds a stop one bit, and adds some trailing zeros
 *        if the resulting number of bits is not multiple of 8
 * 
 * @return Bitstream with a stop one appended, and trailing zeros (if nb_bits % 8 != 0)
 */
Bitstream Bitstream::rbsp_trailing_bits() {
  // rbsp_stop_one_bit
  Bitstream rbsp = (*this) + Bitstream(static_cast<std::uint8_t>(1), 1);
  int trail_bits = rbsp.nb_bits % 8;
  // rbsp_trailing_bits
  if (trail_bits != 0)
    rbsp += Bitstream(static_cast<std::uint8_t>(0), (8-trail_bits));
  return rbsp;
}

/* This function add emulation_prevention_three_byte for all occurrences
 * of the following byte sequences in the stream
 *  0x000000  -> 0x00000300
 *  0x000001  -> 0x00000301
 *  0x000002  -> 0x00000302
 *  0x000003  -> 0x00000303
 */
Bitstream Bitstream::rbsp_to_ebsp() {
  
  assert(nb_bits % 8 == 0);

  // output: ebsp
  Bitstream ebsp;
  int count = 0;

  for (const auto& byte : buffer) {
    // Detect 0x00 twice
    if (count == 2 && !(byte & 0xfc)) {
      ebsp.buffer.push_back(0x03);
      ebsp.nb_bits += 8;
      count = 0;
    }
    ebsp.buffer.push_back(byte);
    ebsp.nb_bits += 8;
    if (byte == 0x00) {
      count++;
    }
    else {
      count = 0;
    }
  }

  return ebsp;
}

/**
 * @brief Converts the bitstream to string
 * 
 * @return std::string containing the set of bits
 * 
 * @note Introduces a space between bytes
 */
std::string Bitstream::to_string() {
  int nb_full_digit = nb_bits / 8;
  int trail_bits = nb_bits % 8;

  std::string s;
  for (int i = 0; i < nb_full_digit; i++) {
    std::bitset<8> bits(buffer[i]);
    s += bits.to_string() + " ";
  }

  if (trail_bits != 0) {
    std::bitset<8> bits(buffer[nb_full_digit]);
    s += bits.to_string().substr(0, trail_bits);
  }

  return s;
}

