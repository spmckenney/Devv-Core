/*
 * ProposedBlock.h
 *
 *  Created on: Jun 19, 2018
 *      Author: Shawn McKenney
 */
#include "ProposedBlock.h"
#include "primitives/json_interface.h"

namespace Devv {

bool ProposedBlock::validate(const KeyRing& keys) const {
  LOG_DEBUG << "validate()";
  MTR_SCOPE_FUNC();
  if (transaction_vector_.size() < 1) {
    LOG_WARNING << "Trying to validate empty block.";
    return false;
  }

  if (!summary_.isSane()) {
    LOG_WARNING << "Summary is invalid in block.validate()!\n";
    LOG_DEBUG << "Summary state: " + GetJSON(summary_);
    return false;
  }

  std::vector<byte> md = summary_.getCanonical();
  for (auto& sig : vals_.getValidationMap()) {
    if (!VerifyByteSig(keys.getKey(sig.first), DevvHash(md), sig.second)) {
      LOG_WARNING << "Invalid block signature";
      LOG_DEBUG << "Block state: " + GetJSON(*this);
      LOG_DEBUG << "Block Node Addr: " + sig.first.getJSON();
      LOG_DEBUG << "Block Node Sig: " + sig.second.getJSON();
      return false;
    }
  }

  return true;
}

} // namespace Devv
