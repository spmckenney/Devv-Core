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
#include "concurrency/DevcashMPMCQueue.h"

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

  /**
   * Stops the server and exits the server thread
   */
  void StopServer();

 private:
  // Initialize ZMQ sockets
  void Run() noexcept;

  // communication urls
  const std::string bind_url_;

  // zmq context
  zmq::context_t& context_;

  // publication socket
  std::unique_ptr<zmq::socket_t> pub_socket_ = nullptr;

  // Used to run the server service in a background thread
  std::unique_ptr<std::thread> server_thread_ = nullptr;

  // Used to queue outgoing messages
  DevcashMPMCQueue message_queue_;

  // Set to true when server thread starts and
  // false when a shutdown is requested
  bool keep_running_ = false;
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

  /**
   * Create a listening filter
   */
  void ListenTo(const std::string& filter);

  /**
   * Stops the client and exits the thread
   */
  void StopClient();

 private:
  // process received message
  void ProcessIncomingMessage() noexcept;

  // ZMQ communication urls
  std::vector<std::string> peer_urls_;

  // ZMQ context reference
  zmq::context_t& context_;

  // subscriber socket
  std::unique_ptr<zmq::socket_t> sub_socket_ = nullptr;

  // Used to run the client service in a background thread
  std::unique_ptr<std::thread> client_thread_ = nullptr;

  // List of callbacks to call when a message arrives
  DevcashMessageCallback callback_;

  // List of strings to filter incoming messages
  std::vector<std::string> filter_vector_;

  // Set to true when client thread starts and
  // false when a shutdown is requested
  bool keep_running_ = false;
};

static inline std::string zero_append(const std::string& number, size_t num_width) {
  if (number.size() >= num_width) return number;

  auto to_pad = num_width - number.size();
  return std::string(to_pad, '0').append(number);
}

  static inline bool synchronize(const std::string& sync_host, unsigned int node_number) {

  //  Prepare our context and socket
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  LOG_INFO << "Connecting to synchronization host (" << sync_host << ")";
  socket.connect("tcp://" + sync_host);

  std::string node_str = "node-"+zero_append(std::to_string(node_number), 3);

  zmq::message_t request (node_str.size());
  memcpy(request.data(), "Hello", node_str.size());

  LOG_INFO << "Sending sync packet ";
  socket.send(request);

  //  Get the reply.
  zmq::message_t reply;
  socket.recv(&reply);
  LOG_DEBUG << "Received sync response - it's go time";
  return true;
}

} // namespace io
} // namespace Devcash
