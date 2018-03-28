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

/**
 * TransactionClient
 */
class TransactionServer final {
 public:
  /**
   * Constructor
   */
  TransactionServer(zmq::context_t& context,
                    const std::string& bind_url);

  // Disable copying
  TransactionServer(const TransactionServer&) = delete;
  TransactionServer& operator=(const TransactionServer&) = delete;

  /**
   * Send a message directly.
   * This must be called from the same thread in which
   * the socket lives. It is public for convenience, but
   * QueueMessage() should probably be used.
   */
  void SendMessage(DevcashMessageUniquePtr message) noexcept;

  /**
   * Queue a message.
   * This can be called from a different thread than
   * the one that owns the socket. However, this method
   * is not thread-safe, so it shouldn't be called from
   * multiple threads simultaneously.
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
  TransactionClient(zmq::context_t& context);

  /// Attach a callback to be called when a DevcashMessage
  /// arrives on the wire.
  void AttachCallback(DevcashMessageCallback);

  // Start the transaction client service
  // Blocks indefinitely
  void Run();

  // Add a devcash node to connect to
  void AddConnection(const std::string& endpoint);

  /**
   * Starts a client in a background thread
   */
  void StartClient();

 private:
  // process received message
  void ProcessIncomingMessage() noexcept;

  // ZMQ communication urls
  std::vector<std::string> peer_urls_;

  // ZMQ context reference
  zmq::context_t& context_;

  // subscriber socket
  std::unique_ptr<zmq::socket_t> sub_socket_;

  // Used to run the client service in a background thread
  std::unique_ptr<std::thread> client_thread_;

  // List of callbacks to call when a message arrives
  DevcashMessageCallback callback_;
};

} // namespace io
} // namespace Devcash
