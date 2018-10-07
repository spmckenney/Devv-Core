/*
 * devv_version.h
 *
 *  Created on: 10/7/18
 *      Author: mckenney
 */
#pragma once
#include <string>

namespace Devv
{

struct Version
{
  static const std::string GIT_SHA1;
  static const std::string GIT_DATE;
  static const std::string GIT_COMMIT_SUBJECT;
  static const std::string GIT_BRANCH;
};

}
