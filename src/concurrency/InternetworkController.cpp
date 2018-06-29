/*
 * InternetworkController.cpp
 *
 *  Created on: 6/22/18
 *      Authors: Nick Williams
 *               Shawn McKenney
 */
#include "concurrency/InternetworkController.h"

#include "common/devcash_exceptions.h"
#include "common/logger.h"
#include "consensus/tier2_message_handlers.h"

namespace Devcash {

InternetworkController::InternetworkController(const KeyRing &keys,
                                         DevcashContext &context,
                                         const ChainState &,
                                         Blockchain &final_chain,
                                         UnrecordedTransactionPool &utx_pool,
                                         eAppMode mode)
    : keys_(keys)
    , context_(context)
    , final_chain_(final_chain)
    , utx_pool_(utx_pool)
    , mode_(mode)
, blocks_since_cb_(HandleBlocksSince)
, blocks_since_request_cb_(HandleBlocksSinceRequest)
{
}

void InternetworkController::messageCallback(DevcashMessageUniquePtr ptr) {
  std::lock_guard<std::mutex> guard(utx_mutex_);
  MTR_SCOPE_FUNC();
  try {
    switch (ptr->message_type) {
      /*
      case eMessageType::FINAL_BLOCK:LOG_WARNING << "Unexpected message @ shard comms, to consensus.\n";
        pushConsensus(std::move(ptr));
        break;

      case eMessageType::PROPOSAL_BLOCK:LOG_WARNING << "Unexpected message @ shard comms, to consensus.\n";
        pushConsensus(std::move(ptr));
        break;

      case eMessageType::TRANSACTION_ANNOUNCEMENT:LOG_WARNING << "Unexpected message @ shard comms, to validator";
        pushValidator(std::move(ptr));
        break;

      case eMessageType::VALID:LOG_WARNING << "Unexpected message @ shard comms, to consensus.\n";
        pushConsensus(std::move(ptr));
        break;
*/
      case eMessageType::REQUEST_BLOCK:
        LOG_DEBUG << "ValidatorController()::ShardCommsCallback(): REQUEST_BLOCK";
        //request updates from remote shards if this chain has grown
        if (remote_blocks_ < final_chain_.size()) {
          std::vector<byte> request;
          Uint64ToBin(remote_blocks_, request);
          uint64_t node = context_.get_current_node();
          Uint64ToBin(node, request);
          if (mode_ == eAppMode::T1) {
            //request blocks from all live remote shards with the node index matching this node's index
            //in the case of benchmarking, there should be two live T2 shards,
            //so assume they are shards 1 and 2 (where T1 has the 0 index)
            std::string uri1 = context_.get_uri_from_index(
                context_.get_peer_count() + context_.get_current_node() % context_.get_peer_count());
            auto blocks_msg1 = std::make_unique<DevcashMessage>(uri1, GET_BLOCKS_SINCE, request, remote_blocks_);
            outgoing_callback_(std::move(blocks_msg1));
            std::string uri2 = context_.get_uri_from_index(
                context_.get_peer_count() * 2 + context_.get_current_node() % context_.get_peer_count());
            auto blocks_msg2 = std::make_unique<DevcashMessage>(uri2, GET_BLOCKS_SINCE, request, remote_blocks_);
            outgoing_callback_(std::move(blocks_msg2));
          } else if (mode_ == eAppMode::T2) {
            //a T2 should request new blocks from the T1 node with the same node index as itself
            std::string uri = context_.get_uri_from_index(context_.get_current_node() % context_.get_peer_count());
            auto blocks_msg = std::make_unique<DevcashMessage>(uri, GET_BLOCKS_SINCE, request, remote_blocks_);
            outgoing_callback_(std::move(blocks_msg));
          } else {
            LOG_WARNING << "Unsupported mode: " << mode_;
          }
        } else {
          LOG_DEBUG << "remote_blocks_ >= final_chain_.size()";
        }
        break;

      case eMessageType::GET_BLOCKS_SINCE:LOG_DEBUG << "ValidatorController()::ShardCommsCallback(): GET_BLOCKS_SINCE";
        //provide blocks since requested height
        blocks_since_request_cb_(std::move(ptr),
                                                   final_chain_,
                                                   context_,
                                                   keys_,
                                                   [this](DevcashMessageUniquePtr p) {
                                                     this->outgoing_callback_(std::move(p));
                                                   });
        break;

      case eMessageType::BLOCKS_SINCE:LOG_DEBUG << "ValidatorController()::ShardCommsCallback(): BLOCKS_SINCE";
        blocks_since_cb_(std::move(ptr),
                                           final_chain_,
                                           context_,
                                           keys_,
                                           utx_pool_,
                                           remote_blocks_);
        break;

      default:LOG_ERROR << "ValidatorController()::ShardCommsCallback(): Unexpected message, ignore.\n";
        break;
    }
  } catch (const DeserializationError &e) {
    LOG_FATAL << FormatException(&e, "ValidatorController.ShardCommsCallback()");
  } catch (...) {
    throw;
  }
}

} // namespace Devcash
