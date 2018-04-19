#include <string>

#include "common/logger.h"
#include "common/util.h"
#include "primitives/Transfer.h"

int main(int argc, char* argv[])
{
  std::string test_addr("02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4");
  Transfer test_transfer(Hex2Bin(test_addr), 0, 1, 0);
  Transfer identity(test_transfer.getCanonical());

  if (test_transfer.getJSON() == identity.getJSON()) {
    LOG_INFO << "Transfer JSON identity pass.";
  } else {
	LOG_WARNING << "Transfer JSON identity fail.";
  }

  if (test_transfer.getCanonical() == identity.getCanonical()) {
    LOG_INFO << "Transfer canonical identity pass.";
  } else {
	LOG_WARNING << "Transfer canonical identity fail.";
  }
}