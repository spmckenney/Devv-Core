/*
 * DevcashMessage.h
 *
 *  Created on: Mar 15, 2018
 *      Author: Silver
 */

#ifndef QUEUES_DEVCASHMESSAGE_H_
#define QUEUES_DEVCASHMESSAGE_H_

#include <string>
#include <vector>

namespace Devcash {

enum eMessageType {
  FINAL_BLOCK = 0,
  PROPOSAL_BLOCK = 1,
  TRANSACTION_ANNOUNCEMENT = 2,
  VALID = 3,
  REQUEST_BLOCK = 4,
  NUM_TYPES = 5
};

typedef std::string URI;

struct DevcashMessage {
  URI uri;
  eMessageType message_type;
  std::vector<uint8_t> data;
  int index;
  DevcashMessage() : uri(""), message_type(eMessageType::VALID), data() {}
  DevcashMessage(URI uri, eMessageType msgType, std::vector<uint8_t> data, int index=0) :
    uri(uri), message_type(msgType), data(data), index(index) {}
};

} /* namespace Devcash */

#endif /* QUEUES_DEVCASHMESSAGE_H_ */
