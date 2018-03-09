/*
 * message_service.cpp - implements messaging
 *
 *  Created on: Mar 3, 2018
 *  Author: Shawn McKenney
 */
#include <io/message_service.h>

#include <vector>

#include <folly/Random.h>
#include <io/constants.h>

namespace Devcash {
namespace io {

TransactionServer::TransactionServer(
    fbzmq::Context& context,
    const std::string& bind_url)
    : bind_url_(bind_url),
      pub_socket_(context),
      serializer_() {
  prepare();
}

void
TransactionServer::SendMessage(DevcashMessageSharedPtr message) noexcept {
  // Create and populate the thrift message
  thrift::DevcashMessage thrift_message;
  thrift_message.uri = message->uri;
  thrift_message.message_type = message_type_to_thrift.at(message->message_type);
  //thrift_message.data 

  auto rc = pub_socket_.sendThriftObj(thrift_message, serializer_);
  if (rc.hasError()) {
    LOG(ERROR) << "Send message failed: " << rc.error();
    return;
  } else {
    LOG(INFO) << "Sent message";
  }
}

void
TransactionServer::prepare() noexcept {

  LOG(INFO) << "Server: Binding bind_url_ '" << bind_url_ << "'";
  pub_socket_.bind(fbzmq::SocketUrl{bind_url_}).value();


  /*
// attach callbacks for thrift type command
  addSocket(
      fbzmq::RawZmqSocketPtr{*thriftCmdSock_},
      ZMQ_POLLIN,
      [this](int) noexcept {
        LOG(INFO) << "Received command request on thriftCmdSock_";
        processThriftCommand();
      });

  // attach callbacks for multiple command
  addSocket(
      fbzmq::RawZmqSocketPtr{*multipleCmdSock_},
      ZMQ_POLLIN,
      [this](int) noexcept {
        LOG(INFO) << "Received command request on multipleCmdSock_";
        processMultipleCommand();
      });
  */
}

/*
 * TransactionClient
 */
TransactionClient::TransactionClient(
    fbzmq::Context& context,
    const std::string& peer_url)
    : context_(context),
      peer_url_(peer_url),
      sub_socket_(context),
      callback_vector_() {
  prepare();
}

void
TransactionClient::prepare() noexcept {
  LOG(INFO) << "Client connecting pubUrl_ '" << peer_url_ << "'";
  sub_socket_.connect(fbzmq::SocketUrl{peer_url_}).value();
}

void
TransactionClient::ProcessIncomingMessage() noexcept {
  // read out thrift command
  auto thrift_object =
      sub_socket_.recvThriftObj<thrift::DevcashMessage>(serializer_);
  if (thrift_object.hasError()) {
    LOG(ERROR) << "read thrift request failed: " << thrift_object.error();
    return;
  } else {
    LOG(INFO) << "Received Thrift Object";
  }
  const auto& thrift_devcash_message = thrift_object.value();

  auto message = MakeDevcashMessage(thrift_devcash_message);

  for (fun : callback_vector_) {
    fun(message);
  }
}

void
TransactionClient::AttachCallback(DevcashMessageCallback callback) {
  callback_vector_.push_back(callback);
}

DevcashMessageSharedPtr
MakeDevcashMessage(const std::string& uri,
                   const thrift::MessageType& message_type,
                   const std::string& data) {

  DataBufferSharedPtr buffer = std::make_shared<DataBuffer>(data.begin(),
                                                            data.end());
  DevcashMessageSharedPtr devcash_message =
    std::make_shared<DevcashMessage>();

  devcash_message->uri = uri;
  //message_type_to_devcash.at(int(message_type)),
  //buffer
  return devcash_message;
}

} // namespace io
} // namespace Devcash
