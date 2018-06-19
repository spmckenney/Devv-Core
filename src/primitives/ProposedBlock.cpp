//
// Created by mckenney on 6/17/18.
//

#include "ProposedBlock.h"
#include "primitives/json_interface.h"

namespace Devcash {

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
    if (!VerifyByteSig(keys.getKey(sig.first), DevcashHash(md), sig.second)) {
      LOG_WARNING << "Invalid block signature";
      LOG_DEBUG << "Block state: " + GetJSON(*this);
      LOG_DEBUG << "Block Node Addr: " + ToHex(std::vector<byte>(std::begin(sig.first), std::end(sig.first)));
      LOG_DEBUG << "Block Node Sig: " + ToHex(std::vector<byte>(std::begin(sig.second), std::end(sig.second)));
      return false;
    }
  }

  return true;
}

} // namespace Devcash
