/*
 * message_service.h - Sending and receiving Devcash messages
 *
 *  Created on: Mar 3, 2018
 *  Created by: Shawn McKenney
 */
#pragma once

#include "io/zhelpers.hpp"

#include "types/DevcashMessage.h"
#include "concurrency/DevcashSPSCQueue.h"

namespace Devcash {
namespace io {

typedef std::function<void(DevcashMessageUniquePtr)> DevcashMessageCallback;

class TransactionServer final {
 public:
  TransactionServer(
      zmq::context_t& context,
      const std::string& bind_url);

  // disable copying
  TransactionServer(const TransactionServer&) = delete;
  TransactionServer& operator=(const TransactionServer&) = delete;

  // Send a message
  void SendMessage(DevcashMessageUniquePtr message) noexcept;

  /**
   * Queue a message
   */
  void QueueMessage(DevcashMessageUniquePtr message) noexcept;

  /**
   * Starts a server in a background thread
   */
  void StartServer();

 private:
  // Initialize ZMQ sockets
  void Run() noexcept;

  // communication urls
  const std::string bind_url_;

  // zmq context
  zmq::context_t& context_;

  // publication socket
  std::unique_ptr<zmq::socket_t> pub_socket_;

  // Used to run the server service in a background thread
  std::unique_ptr<std::thread> server_thread_;

  // Used to queue outgoing messages
  DevcashSPSCQueue message_queue_;
};

/**
 * TransactionClient
 */
class TransactionClient final {
 public:
  TransactionClient(
      zmq::context_t& context,
      const std::string& peer_url);

  /// Attach a callback to be called when a DevcashMessage
  /// arrives on the wire.
  void AttachCallback(DevcashMessageCallback);

  // Start the transaction client service
  void Run();

 private:
  // Initialize ZMQ sockets
  void prepare() noexcept;

  // process received message
  void ProcessIncomingMessage() noexcept;

  // ZMQ communication urls
  const std::string peer_url_;

  // ZMQ context reference
  zmq::context_t& context_;

  // subscriber socket
  zmq::socket_t sub_socket_;

  /// List of callbacks to call when a message arrives
  DevcashMessageCallback callback_vector_;
};

} // namespace io
} // namespace Devcash
