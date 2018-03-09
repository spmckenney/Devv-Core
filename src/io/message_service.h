/*
 * message_service.h - Sending and receiving Devcash messages
 *
 *  Created on: Mar 3, 2018
 *  Created by: Shawn McKenney
 */
#pragma once

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <fbzmq/async/ZmqEventLoop.h>
#include <fbzmq/zmq/Zmq.h>

#include <thrift/gen-cpp2/Devcash_types.h>

#include "common/types.h"

namespace Devcash {
namespace io {

typedef std::function<void(DevcashMessageSharedPtr)> DevcashMessageCallback;

class TransactionServer final : public fbzmq::ZmqEventLoop {
 public:
  TransactionServer(
      fbzmq::Context& context,
      const std::string& bind_url);

  // disable copying
  TransactionServer(const TransactionServer&) = delete;
  TransactionServer& operator=(const TransactionServer&) = delete;

  // Send a message
  void SendMessage(DevcashMessageSharedPtr message) noexcept;

 private:
  // Initialize ZMQ sockets
  void prepare() noexcept;

  // communication urls
  const std::string bind_url_;

  // publication socket
  fbzmq::Socket<ZMQ_PUB, fbzmq::ZMQ_SERVER> pub_socket_;

  // used for serialize/deserialize thrift obj
  apache::thrift::CompactSerializer serializer_;
};

class TransactionClient final : public fbzmq::ZmqEventLoop {
 public:
  TransactionClient(
      fbzmq::Context& context,
      const std::string& peer_url);

  /// Attach a callback to be called when a DevcashMessage
  /// arrives on the wire.
  void AttachCallback(DevcashMessageCallback);

 private:
  // Initialize ZMQ sockets
  void prepare() noexcept;

  // process received message
  void ProcessIncomingMessage() noexcept;

  // ZMQ context reference
  fbzmq::Context& context_;

  // ZMQ communication urls
  const std::string peer_url_;

  // subscriber socket
  fbzmq::Socket<ZMQ_SUB, fbzmq::ZMQ_CLIENT> sub_socket_;

  // used for serialize/deserialize thrift obj
  apache::thrift::CompactSerializer serializer_;

  /// List of callbacks to call when a message arrives
  std::vector<DevcashMessageCallback> callback_vector_;
};

  // Create a static enum map
static std::vector<thrift::MessageType> message_type_to_thrift = {
  thrift::MessageType::KEY_FINAL_BLOCK,
  thrift::MessageType::KEY_PROPOSAL_BLOCK,
  thrift::MessageType::KEY_TRANSACTION_ANNOUNCEMENT,
  thrift::MessageType::KEY_VALID,
};
static std::vector<Devcash::MessageType> message_type_to_devcash = {
  Devcash::MessageType::eFinalBlock,
  Devcash::MessageType::eProposalBlock,
  Devcash::MessageType::eTransactionAnnouncement,
  Devcash::MessageType::eValid,
};

DevcashMessageSharedPtr MakeDevcashMessage(const std::string& uri,
                                           const thrift::MessageType& message_type,
                                           const std::string& data);

template <typename ThriftObject>
DevcashMessageSharedPtr MakeDevcashMessage(const ThriftObject& object) {
  return MakeDevcashMessage(object.uri,
                            object.message_type,
                            object.data);
}

} // namespace io
} // namespace Devcash
