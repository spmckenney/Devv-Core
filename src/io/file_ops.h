/*
 * file_ops.h
 *
 *  Created on: 8/9/18
 *      Author: Shawn McKenney
 */
#pragma once

#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <iterator>

#include <boost/filesystem.hpp>

#include "common/devv_types.h"

namespace fs = boost::filesystem;

namespace Devv {

struct key_tuple {
  std::string address;
  std::string key;
};

std::vector<byte> ReadBinaryFile(const fs::path& path);

std::string ReadTextFile(const fs::path& file_path);

struct key_tuple ReadKeyFile(const fs::path& path);

} // namespace Devv
