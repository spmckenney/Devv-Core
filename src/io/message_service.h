/*
 * message_service.h - Sending and receiving Devcash messages
 *
 *  Created on: Mar 3, 2018
 *  Created by: Shawn McKenney
 */
#pragma once

#include "zhelpers.hpp"
#include "concurrency/DevcashSPSCQueue.h"
#include "concurrency/DevcashWorkerPool.h"

namespace Devcash {

namespace io {

typedef std::function<void(std::unique_ptr<DevcashMessage>)> DevcashMessageCallback;

class TransactionServer final {
 public:
  TransactionServer(
      zmq::context_t& context,
      const std::string& bind_url);

  // Disable copying
  TransactionServer(const TransactionServer&) = delete;
  TransactionServer& operator=(const TransactionServer&) = delete;

  // Send a message
  void SendMessage(std::unique_ptr<DevcashMessage> message) noexcept;

  /**
   * Queue a message
   */
  void QueueMessage(std::unique_ptr<DevcashMessage> message) noexcept;

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
  TransactionClient(zmq::context_t& context);

  /// Attach a callback to be called when a DevcashMessage
  /// arrives on the wire.
  void AttachCallback(DevcashMessageCallback);

  // Start the transaction client service
  // Blocks indefinitely
  void Run();

  // Add a devcash node to connect to
  void AddConnection(const std::string& endpoint);

 private:
  // process received message
  void ProcessIncomingMessage() noexcept;

  // ZMQ communication urls
  std::vector<std::string> peer_urls_;

  // ZMQ context reference
  zmq::context_t& context_;

  // subscriber socket
  std::unique_ptr<zmq::socket_t> sub_socket_;

  /// List of callbacks to call when a message arrives
  DevcashMessageCallback callback_;
};

} // namespace io
} // namespace Devcash
