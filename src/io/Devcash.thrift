/**
 * Copyright 2014-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the license found in the
 * LICENSE-examples file in the root directory of this source tree.
 */

namespace cpp2 Devcash.io.thrift

enum MessageType {
  KEY_FINAL_BLOCK = 1,
  KEY_PROPOSAL_BLOCK = 2,
  KEY_TRANSACTION_ANNOUNCEMENT = 3,
  KEY_VALID = 4,
}

typedef string URI

struct DevcashMessage {
  1: required URI remote,
  2: required MessageType message_type,
  3: required binary data,
}

