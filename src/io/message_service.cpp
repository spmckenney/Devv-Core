/*
 * message_service.cpp - implements messaging
 *
 *  Created on: Mar 3, 2018
 *  Author: Shawn McKenney
 */
#include <io/message_service.h>

#include <vector>

#include <io/constants.h>

namespace Devcash {
namespace io {

TransactionServer::TransactionServer(
    zmq::context_t& context,
    const std::string& bind_url)
    : bind_url_(bind_url)
    , context_(context)
    , pub_socket_(nullptr)
    , server_thread_(nullptr) {
}

void
TransactionServer::SendMessage(DevcashMessageUniquePtr dc_message) noexcept {
  LOG_DEBUG << "SendMessage(): Sending message: " << dc_message->uri;
  auto buffer = serialize(*dc_message);
  s_sendmore(*pub_socket_, dc_message->uri);
  s_send(*pub_socket_, buffer);
}

void
TransactionServer::QueueMessage(DevcashMessageUniquePtr message) noexcept
{
  LOG_DEBUG << "QueueMessage: Queueing()";
  message_queue_.push(std::move(message));
}

void
TransactionServer::StartServer() {
  LOG_DEBUG << "Starting TransactionServer";
  server_thread_ = std::unique_ptr<std::thread>(new std::thread([this]() { this->Run(); }));
}

void
TransactionServer::Run() noexcept {

  pub_socket_ = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(context_, ZMQ_PUB));
  LOG_INFO << "Server: Binding bind_url_ '" << bind_url_ << "'";
  pub_socket_->bind(bind_url_);

  for (;;) {
    auto message = message_queue_.pop();
    SendMessage(std::move(message));
  }
}

/*
 * TransactionClient
 */
TransactionClient::TransactionClient(zmq::context_t& context)
  : peer_urls_()
  , context_(context)
  , sub_socket_(nullptr)
  , callback_() {
}

void
TransactionClient::AddConnection(const std::string& endpoint) {
  peer_urls_.push_back(endpoint);
}

void
TransactionClient::ProcessIncomingMessage() noexcept {
  LOG_DEBUG << "ProcessIncomingMessage(): Waiting for message";
  /* Block until a message is available to be received from socket */

  auto uri = s_recv(*sub_socket_);
  LOG_DEBUG << "Received - envelope message: " << uri;
  auto devcash_message = deserialize(s_vrecv(*sub_socket_));
  LOG_DEBUG << "ProcessIncomingMessage(): Received a message";

  LogDevcashMessageSummary(*devcash_message);

  callback_(std::move(devcash_message));
}

void
TransactionClient::Run() {
  sub_socket_ = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(context_, ZMQ_SUB));

  for (auto endpoint : peer_urls_) {
    sub_socket_->connect(endpoint);
    for ( auto filter : filter_vector_) {
      LOG_DEBUG << "ZMQ_SUBSCRIBE: '" << endpoint
                << ":" << filter << "'";
      sub_socket_->setsockopt(ZMQ_SUBSCRIBE, filter.c_str(), filter.size());
    }

  }

  for (;;) {
    ProcessIncomingMessage();
  }
}

void
TransactionClient::StartClient() {
  LOG_DEBUG << "Starting TransactionClient thread";
  client_thread_ = std::make_unique<std::thread>([this]() { this->Run(); });
}

void
TransactionClient::ListenTo(const std::string& filter) {
  filter_vector_.push_back(filter);
}

void
TransactionClient::AttachCallback(DevcashMessageCallback callback) {
  callback_ = callback;
}

} // namespace io
} // namespace Devcash
