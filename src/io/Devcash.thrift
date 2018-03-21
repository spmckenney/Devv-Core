/**
 * Copyright 2014-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the license found in the
 * LICENSE-examples file in the root directory of this source tree.
 */

namespace cpp2 Devcash.io.thrift

enum MessageType {
  KEY_FINAL_BLOCK = 0,
  KEY_PROPOSAL_BLOCK = 1,
  KEY_TRANSACTION_ANNOUNCEMENT = 2,
  KEY_VALID = 3,
  KEY_REQUEST_BLOCK = 4,
  KEY_NUM_TYPES = 5,
}

typedef string URI

struct DevcashMessage {
  1: required URI uri,
  2: required MessageType message_type,
  3: required binary data,
}
