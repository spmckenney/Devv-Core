/*
 * message_service.h - Sending and receiving Devcash messages
 *
 *  Created on: Mar 3, 2018
 *  Created by: Shawn McKenney
 */
#pragma once

#include "io/zhelpers.hpp"

#include "concurrency/DevcashMPMCQueue.h"
#include "concurrency/DevcashSPSCQueue.h"
#include "types/DevcashMessage.h"

namespace Devcash {

namespace io {

/**
 * TransactionClient
 */
class TransactionServer final {
 public:
  /**
   * Constructor
   * @param context
   * @param bind_url
   */
  TransactionServer(zmq::context_t& context,
                    const std::string& bind_url,
                    int socket_protocol = ZMQ_PUB);

  // Disable copying
  TransactionServer(const TransactionServer&) = delete;
  TransactionServer& operator=(const TransactionServer&) = delete;

  /**
   * Send a message directly.
   * This must be called from the same thread in which
   * the socket lives. It is public for convenience, but
   * QueueMessage() should probably be used.
   * @param message messge to send
   */
  void sendMessage(DevcashMessageUniquePtr message) noexcept;

  /**
   * Queue a message.
   * This can be called from a different thread than
   * the one that owns the socket. However, this method
   * is not thread-safe, so it shouldn't be called from
   * multiple threads simultaneously.
   * @param message message to queue
   */
  void queueMessage(DevcashMessageUniquePtr message) noexcept;

  /**
   * Starts a server in a background thread
   */
  void startServer();

  /**
   * Stops the server and exits the server thread
   */
  void stopServer();

 private:
  /**
   * Starts the thread and send loop
   */
  void run() noexcept;

  /// URL to bind to
  const std::string bind_url_;

  /// zmq context
  zmq::context_t& context_;

  /// publication socket
  std::unique_ptr<zmq::socket_t> pub_socket_ = nullptr;

  /// Used to run the server service in a background thread
  std::unique_ptr<std::thread> server_thread_ = nullptr;

  /// The protocol (pub/sub - push/pull) for the zmq socket
  int socket_protocol_ = ZMQ_PUB;

  /// The zmq socket flags
  int socket_flags_ = 0;

  /// Used to queue outgoing messages
  DevcashMPMCQueue message_queue_;

  // Set to true when server thread starts and false when a shutdown is requested
  bool keep_running_ = false;
};

/**
 * TransactionClient
 */
class TransactionClient final {
 public:
  TransactionClient(zmq::context_t& context, int socket_protocol = ZMQ_SUB);

  ~TransactionClient() {
    /// Stop the thread if it is running
    stopClient();
  }

  /**
   * Attach a callback to be called when a DevcashMessage arrives on the wire.
   * @param callback to call when a message arrives
   */
  void attachCallback(DevcashMessageCallback callback);

  /**
   * Start the transaction client service. Blocks until stopClient() is called.
   */
  void run();

  /**
   * Add a Devcash node to connect to
   * @param endpoint URI to connect to
   */
  void addConnection(const std::string& endpoint);

  /**
   * Starts a client in a background thread
   */
  void startClient();

  /**
   * Create a listening filter
   * @param filter zmq incoming message filter
   */
  void listenTo(const std::string& filter);

  /**
   * Stops the client.
   * keep_running_ is set to false causing run() to return
   */
  void stopClient();

 private:
  /**
   * process received message
   */
  void processIncomingMessage() noexcept;

  /// ZMQ communication urls. The client will connect to each URI and subscribe
  std::vector<std::string> peer_urls_ = {};

  /// ZMQ context reference
  zmq::context_t& context_;

  /// pointer to subscriber socket
  std::unique_ptr<zmq::socket_t> sub_socket_ = nullptr;

  /// Used to run the client service in a background thread
  std::unique_ptr<std::thread> client_thread_ = nullptr;

  /// List of callbacks to call when a message arrives
  DevcashMessageCallback callback_;

  /// List of strings to filter incoming messages
  std::vector<std::string> filter_vector_;

  /// The protocol (pub/sub - push/pull) for the zmq socket
  int socket_protocol_ = ZMQ_SUB;

  /// Set to true when client thread starts and
  /// false when a shutdown is requested
  bool keep_running_ = false;
};

/**
 * If the number of numerals in number is less than num_width, number is zero-padded
 * and returned. If the lenth of number is greater than num_width, number is returned.
 * @param number
 * @param num_width
 * @return zero-padded number
 */
static inline std::string zeroPrepend(const std::string& number, size_t num_width) {
  if (number.size() >= num_width) { return number; }

  auto to_pad = num_width - number.size();
  return std::string(to_pad, '0').append(number);
}

/**
 * Create the TransactionClient and add the host connections
 * @param host_vector
 * @param context
 * @return
 */
std::unique_ptr<io::TransactionClient> CreateTransactionClient(const std::vector<std::string>& host_vector,
                                                               zmq::context_t& context);

/**
 * Creates a server and binds to the bind_endpoint to listen for
 * incoming connections
 *
 * @param bind_endpoint
 * @param context
 * @return
 */
std::unique_ptr<io::TransactionServer> CreateTransactionServer(const std::string& bind_endpoint,
                                                               zmq::context_t& context);

}  // namespace io
}  // namespace Devcash
