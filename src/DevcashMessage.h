/*
 * DevcashMessage.h
 *
 *  Created on: Mar 15, 2018
 *      Author: Silver
 */

#ifndef QUEUES_DEVCASHMESSAGE_H_
#define QUEUES_DEVCASHMESSAGE_H_

#include <stdint.h>
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
  DevcashMessage(URI uri, eMessageType msgType, std::vector<uint8_t> data) :
    uri(uri), message_type(msgType), data(data) {}
};

static std::vector<uint8_t> serialize(DevcashMessage msg) {
  uint8_t typeByte = static_cast<uint8_t>(msg.message_type);
  std::vector<uint8_t> out = {typeByte};
  out.insert(std::end(out), std::begin(msg.data), std::end(msg.data));
  return(out);
}

static DevcashMessage deserialize(std::vector<uint8_t> bytes, std::string uri) {
  eMessageType msgType = static_cast<eMessageType>(bytes[0]);
  bytes.erase(std::begin(bytes));
  return(DevcashMessage(uri, msgType, bytes));
}

} /* namespace Devcash */

#endif /* QUEUES_DEVCASHMESSAGE_H_ */
