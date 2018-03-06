/*
 * message_service.h - Sneding and receiving Devcash messages
 *
 *  Created on: Mar 3, 2018
 *  Author: Shawn McKenney
 */
#pragma once

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <fbzmq/async/ZmqEventLoop.h>
#include <fbzmq/zmq/Zmq.h>

#include "common/types.h"

namespace Devcash {
namespace io {

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

  //void AttachCallback(
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

class TransactionClient {
 public:
  TransactionClient(
      fbzmq::Context& context,
      const std::string& peer_url);

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
};

} // namespace io
} // namespace Devcash
