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
  // Create and populate the thrift message

  //zmq::message_t uri(&(message->uri), sizeof(uri));
  s_send(*pub_socket_, dc_message->uri);

  /*
  zmq::message_t message(sizeof(dc_message->message_type));
  memcpy(message.data(), &dc_message->message_type, sizeof(dc_message->message_type));
  pub_socket_.send(message);
  */

  //zmq_send(&pub_socket_, &message->message_type, sizeof(message->message_type), 0);

  /*
  auto rc = pub_socket_.sendThriftObj(thrift_message, serializer_);
  if (rc.hasError()) {
    LOG(ERROR) << "Send message failed: " << rc.error();
    return;
  } else {
    LOG(info) << "Sent message";
  }
  */
}

void
TransactionServer::QueueMessage(DevcashMessageUniquePtr message) noexcept
{
  message_queue_.push(std::move(message));
}

void
TransactionServer::StartServer() {
  server_thread_ = std::unique_ptr<std::thread>(new std::thread([this]() { this->Run(); }));
}

void
TransactionServer::Run() noexcept {

  pub_socket_ = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(context_, ZMQ_PUB));
  LOG(info) << "Server: Binding bind_url_ '" << bind_url_ << "'";
  pub_socket_->bind(bind_url_);


  for (;;) {
    auto message = message_queue_.pop();
    SendMessage(std::move(message));
  }

  /*
// attach callbacks for thrift type command
  addSocket(
      fbzmq::RawZmqSocketPtr{*thriftCmdSock_},
      ZMQ_POLLIN,
      [this](int) noexcept {
        LOG(info) << "Received command request on thriftCmdSock_";
        processThriftCommand();
      });

  // attach callbacks for multiple command
  addSocket(
      fbzmq::RawZmqSocketPtr{*multipleCmdSock_},
      ZMQ_POLLIN,
      [this](int) noexcept {
        LOG(info) << "Received command request on multipleCmdSock_";
        processMultipleCommand();
      });
  */
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

  int more;
  //size_t more_size = sizeof(more);

  auto devcash_message = DevcashMessageUniquePtr(new DevcashMessage());
  
/* Create an empty ØMQ message to hold the message part */
    zmq_msg_t part;
    int rc = zmq_msg_init(&part);
    assert (rc == 0);

    LOG(info) << "Waiting for uri";
    /* Block until a message is available to be received from socket */
    zmq::message_t message;
    sub_socket_->recv(&message);

    int size = message.size();
    std::string uri(static_cast<char *>(message.data()), size);
    LOG(info) << "uri: " << uri;

    //rc = zmq_recv (&sub_socket_, &devcash_message->uri_size, 0);
    //assert (rc == 0);
    /* Determine if more message parts are to follow */
    /*
    rc = zmq_getsockopt (&sub_socket_, ZMQ_RCVMORE, &more, &more_size);
    LOG(info) << "rc: " << rc;
    LOG(info) << "more: " << more;
    assert (rc == 1);
   
    rc = zmq_recv(&sub_socket_, &devcash_message->message_type,
                  sizeof(devcash_message->message_type), 0);
    */
    /*
    auto thrift_object =
      sub_socket_.recvThriftObj<thrift::DevcashMessage>(serializer_);
  if (thrift_object.hasError()) {
    LOG(ERROR) << "read thrift request failed: " << thrift_object.error();
    return;
  } else {
    LOG(info) << "Received Thrift Object";
  }
  const auto& thrift_devcash_message = thrift_object.value();

  auto message = MakeDevcashMessage(thrift_devcash_message);
    */
    callback_(std::move(devcash_message));
}

void
TransactionClient::Run() {
  sub_socket_ = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(context_, ZMQ_SUB));
  sub_socket_->setsockopt( ZMQ_SUBSCRIBE, "", 0);

  for (auto endpoint : peer_urls_) {
    sub_socket_->connect(endpoint);
  }

  for (;;) {
    ProcessIncomingMessage();
  }
}

void
TransactionClient::AttachCallback(DevcashMessageCallback callback) {
  callback_ = callback;
}

} // namespace io
} // namespace Devcash
