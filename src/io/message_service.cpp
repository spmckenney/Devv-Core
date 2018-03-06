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
#include <thrift/gen-cpp2/Devcash_types.h>

namespace Devcash {
namespace io {

TransactionServer::TransactionServer(
    fbzmq::Context& context,
    const std::string& bind_url)
    : bind_url_(bind_url),
      pub_socket_(context) {
  prepare();
}

void
TransactionServer::SendMessage(DevcashMessageSharedPtr message) noexcept {
  // Create a static enum map
  std::vector<thrift::MessageType> message_type_ref = {
    thrift::MessageType::KEY_FINAL_BLOCK,
    thrift::MessageType::KEY_PROPOSAL_BLOCK,
    thrift::MessageType::KEY_TRANSACTION_ANNOUNCEMENT,
    thrift::MessageType::KEY_VALID,
  };

  // Create and populate the thrift message
  thrift::DevcashMessage thrift_message;
  thrift_message.uri = message->uri;
  thrift_message.message_type = message_type_ref[message->message_type];
  //thrift_message.data 

  auto rc = pub_socket_.sendThriftObj(thrift_message, serializer_);
  if (rc.hasError()) {
    LOG(ERROR) << "Sent response failed: " << rc.error();
    return;
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
      sub_socket_(context) {
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
  }

  const auto& devcash_message = thrift_object.value();
  const auto& uri = devcash_message.uri;
  //const auto& key = request.key;
  //const auto& value = request.value;

}

} // namespace io
} // namespace Devcash
