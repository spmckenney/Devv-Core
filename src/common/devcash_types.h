/*
 * types/devcash_types.h
 *
 *  Created on: Jun 6, 2018
 *      Author: Shawn McKenney
 */
#pragma once

#include <cstdint>
#include <cstdlib>
#include <array>

namespace Devcash {

typedef unsigned char byte;

// from ossl's sha.h SHA256_DIGEST_LENGTH
const size_t kHASH_LENGTH = 32;

typedef std::array<byte, kHASH_LENGTH> Hash;

enum eAppMode {
  T1 = 0,
  T2 = 1,
  scan = 2
};

} // namespace Devcash