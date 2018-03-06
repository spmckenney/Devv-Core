/*
 * types.h defines basic Devcash types
 *
 *  Created on: Mar 4, 2018
 *  Author: Shawn McKenney
 */

#ifndef DEVCASH_COMMON_TYPES_H
#define DEVCASH_COMMON_TYPES_H

#include <memory>
#include <vector>
#include <cstdint>

namespace Devcash {
  
enum MessageType {
  eFinalBlock = 0,
  eProposalBlock = 1,
  eTransactionAnnouncement = 2,
  eValid = 3,
  eNumTypes = 4,
};

typedef std::vector<uint8_t> DataBuffer;
typedef std::shared_ptr<DataBuffer> DataBufferSharedPtr;

struct DevcashMessage
{
  std::string uri;
  MessageType message_type;
  DataBufferSharedPtr data;
};

typedef std::shared_ptr<DevcashMessage> DevcashMessageSharedPtr;

} // namespace Devcash
#endif // DEVCASH_COMMON_TYPES_H
