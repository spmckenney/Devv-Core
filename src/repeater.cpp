/*
 * repeater.cpp listens for FinalBlock messages and saves them to a file
 * @TODO(nick@cloudsolar.co): it also provides specific blocks by request.
 *
 *  Created on: June 25, 2018
 *  Author: Nick Williams
 */

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "common/logger.h"
#include "common/devcash_context.h"
#include "common/devv_uri.h"
#include "io/message_service.h"
#include "modules/BlockchainModule.h"
#include "pbuf/devv_pbuf.h"

using namespace Devcash;

namespace fs = boost::filesystem;

#define DEBUG_TRANSACTION_INDEX (processed + 11000000)
typedef std::chrono::milliseconds millisecs;

/**
 * Holds command-line options
 */
struct repeater_options {
  std::vector<std::string> host_vector{};
  std::string protobuf_endpoint;
  eAppMode mode = eAppMode::T1;
  unsigned int node_index = 0;
  unsigned int shard_index = 0;
  std::string working_dir;
  std::string inn_keys;
  std::string node_keys;
  std::string key_pass;
  std::string stop_file;
  eDebugMode debug_mode = eDebugMode::off;
};

/**
 * Parse command-line options
 * @param argc
 * @param argv
 * @return
 */
std::unique_ptr<struct repeater_options> ParseRepeaterOptions(int argc, char** argv);

/**
 * Handle repeater request operations.
 * See the Repeater Interface page of the wiki for further information.
 * @param request - a smart pointer to the request
 * @return RepeaterResponsePtr - a smart pointer to the response
 */
RepeaterResponsePtr HandleRepeaterRequest(const RepeaterRequestPtr& request, const std::string& working_dir);

/**
 * Generate a bad syntax error response.
 * @param message - an error message to include with the response
 * @return RepeaterResponsePtr - a smart pointer to the response
 */
RepeaterResponsePtr GenerateBadSyntaxResponse(std::string message) {
  RepeaterResponse response;
  response.return_code = 1010;
  response.message = message;
  return std::make_unique<RepeaterResponse>(response);
}

int main(int argc, char* argv[]) {
  init_log();

  try {
    auto options = ParseRepeaterOptions(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t zmq_context(1);

    DevcashContext this_context(options->node_index, options->shard_index, options->mode, options->inn_keys,
                                options->node_keys, options->key_pass);
    KeyRing keys(this_context);

    LOG_DEBUG << "Write transactions to " << options->working_dir;
    MTR_SCOPE_FUNC();

    fs::path p(options->working_dir);

    if (!is_directory(p)) {
      LOG_ERROR << "Error opening dir: " << options->working_dir << " is not a directory";
      return false;
    }

    std::string shard_name = "Shard-"+std::to_string(options->shard_index);

    //@todo(nick@cloudsolar.co): read pre-existing chain
    unsigned int chain_height = 0;

    auto peer_listener = io::CreateTransactionClient(options->host_vector, zmq_context);
    peer_listener->attachCallback([&](DevcashMessageUniquePtr p) {
      if (p->message_type == eMessageType::FINAL_BLOCK) {
        //write final chain to file
        std::string shard_dir(options->working_dir+"/"+this_context.get_shard_uri());
        fs::path dir_path(shard_dir);
        if (is_directory(dir_path)) {
          std::string block_height(std::to_string(chain_height));
          std::string out_file(shard_dir + "/" + block_height + ".blk");
          std::ofstream block_file(out_file
            , std::ios::out | std::ios::binary);
          if (block_file.is_open()) {
            block_file.write((const char*) &p->data[0], p->data.size());
            block_file.close();
            LOG_DEBUG << "Wrote to " << out_file << "'.";
          } else {
            LOG_ERROR << "Failed to open output file '" << out_file << "'.";
          }
        } else {
          LOG_ERROR << "Error opening dir: " << shard_dir << " is not a directory";
        }
        chain_height++;
      }
    });
    peer_listener->listenTo(this_context.get_shard_uri());
    peer_listener->startClient();
    LOG_INFO << "Repeater is listening to shard: "+this_context.get_shard_uri();

    zmq::context_t context(1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind (options->protobuf_endpoint);

    unsigned int queries_processed = 0;
    while (true) {
      /* Should we shutdown? */
      if (fs::exists(options->stop_file)) {
        LOG_INFO << "Shutdown file exists. Stopping repeater...";
        break;
      }

      zmq::message_t request_message;
      //  Wait for next request from client

      LOG_INFO << "Waiting for RepeaterRequest";
      auto res = socket.recv(&request_message);
      if (!res) {
        LOG_ERROR << "socket.recv != true - exiting";
        break;
      }
      LOG_INFO << "Received Message";
      std::string msg_string = std::string(static_cast<char*>(request_message.data()),
	            request_message.size());

      std::string response;
      RepeaterRequestPtr request_ptr;
      try {
        request_ptr = DeserializeRepeaterRequest(msg_string);
      } catch (std::runtime_error& e) {
        RepeaterResponsePtr response_ptr =
          GenerateBadSyntaxResponse("RepeaterRequeste Deserialization error: "
          + std::string(e.what()));
        std::stringstream response_ss;
        Devv::proto::RepeaterResponse pbuf_response = SerializeRepeaterResponse(std::move(response_ptr));
        pbuf_response.SerializeToOstream(&response_ss);
        response = response_ss.str();
        zmq::message_t reply(response.size());
        memcpy(reply.data(), response.data(), response.size());
        socket.send(reply);
        continue;
      }

      RepeaterResponsePtr response_ptr = HandleRepeaterRequest(std::move(request_ptr), options->working_dir);
      Devv::proto::RepeaterResponse pbuf_response = SerializeRepeaterResponse(std::move(response_ptr));
      LOG_INFO << "Generated RepeaterResponse, Serializing";
      std::stringstream response_ss;
      pbuf_response.SerializeToOstream(&response_ss);
      response = response_ss.str();
      zmq::message_t reply(response.size());
      memcpy(reply.data(), response.data(), response.size());
      socket.send(reply);
      queries_processed++;
      LOG_INFO << "RepeaterResponse sent, process has handled: "
                +std::to_string(queries_processed)+" queries";
    }
    peer_listener->stopClient();
    return (true);
  }
  catch (const std::exception& e) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: " + err << std::endl;
    std::cerr << err << std::endl;
    return (false);
  }
}

std::unique_ptr<struct repeater_options> ParseRepeaterOptions(int argc, char** argv) {

  namespace po = boost::program_options;

  std::unique_ptr<repeater_options> options(new repeater_options());
  std::vector<std::string> config_filenames;

  try {
    po::options_description general("General Options\n\
" + std::string(argv[0]) + " [OPTIONS] \n\
Listens for FinalBlock messages and saves them to a file\n\
\nAllowed options");
    general.add_options()
        ("help,h", "produce help message")
        ("version,v", "print version string")
        ("config", po::value(&config_filenames), "Config file where options may be specified (can be specified more than once)")
        ;

    po::options_description behavior("Identity and Behavior Options");
    behavior.add_options()
        ("debug-mode", po::value<std::string>(), "Debug mode (on|toy|perf) for testing")
        ("mode", po::value<std::string>(), "Devcash mode (T1|T2|scan)")
        ("node-index", po::value<unsigned int>(), "Index of this node")
        ("shard-index", po::value<unsigned int>(), "Index of this shard")
        ("num-consensus-threads", po::value<unsigned int>(), "Number of consensus threads")
        ("num-validator-threads", po::value<unsigned int>(), "Number of validation threads")
        ("host-list,host", po::value<std::vector<std::string>>(),
         "Client URI (i.e. tcp://192.168.10.1:5005). Option can be repeated to connect to multiple nodes.")
        ("protobuf-endpoint", po::value<std::string>(), "Endpoint for protobuf server (i.e. tcp://*:5557)")
        ("working-dir", po::value<std::string>(), "Directory where inputs are read and outputs are written")
        ("output", po::value<std::string>(), "Output path in binary JSON or CBOR")
        ("trace-output", po::value<std::string>(), "Output path to JSON trace file (Chrome)")
        ("inn-keys", po::value<std::string>(), "Path to INN key file")
        ("node-keys", po::value<std::string>(), "Path to Node key file")
        ("key-pass", po::value<std::string>(), "Password for private keys")
        ("stop-file", po::value<std::string>(), "A file in working-dir indicating that this node should stop.")
        ;

    po::options_description all_options;
    all_options.add(general);
    all_options.add(behavior);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
                  options(all_options).
                  run(),
              vm);

    if (vm.count("help")) {
      LOG_INFO << all_options;
      return nullptr;
    }

    if(vm.count("config") > 0)
    {
      config_filenames = vm["config"].as<std::vector<std::string> >();

      for(size_t i = 0; i < config_filenames.size(); ++i)
      {
        std::ifstream ifs(config_filenames[i].c_str());
        if(ifs.fail())
        {
          LOG_ERROR << "Error opening config file: " << config_filenames[i];
          return nullptr;
        }
        po::store(po::parse_config_file(ifs, all_options), vm);
      }
    }

    po::store(po::parse_command_line(argc, argv, all_options), vm);
    po::notify(vm);

    if (vm.count("mode")) {
      std::string mode = vm["mode"].as<std::string>();
      if (mode == "SCAN") {
        options->mode = scan;
      } else if (mode == "T1") {
        options->mode = T1;
      } else if (mode == "T2") {
        options->mode = T2;
      } else {
        LOG_WARNING << "unknown mode: " << mode;
      }
      LOG_INFO << "mode: " << options->mode;
    } else {
      LOG_INFO << "mode was not set.";
    }

    if (vm.count("debug-mode")) {
      std::string debug_mode = vm["debug-mode"].as<std::string>();
      if (debug_mode == "on") {
        options->debug_mode = on;
      } else if (debug_mode == "toy") {
        options->debug_mode = toy;
      } else {
        options->debug_mode = off;
      }
      LOG_INFO << "debug_mode: " << options->debug_mode;
    } else {
      LOG_INFO << "debug_mode was not set.";
    }

    if (vm.count("node-index")) {
      options->node_index = vm["node-index"].as<unsigned int>();
      LOG_INFO << "Node index: " << options->node_index;
    } else {
      LOG_INFO << "Node index was not set.";
    }

    if (vm.count("shard-index")) {
      options->shard_index = vm["shard-index"].as<unsigned int>();
      LOG_INFO << "Shard index: " << options->shard_index;
    } else {
      LOG_INFO << "Shard index was not set.";
    }

    if (vm.count("host-list")) {
      options->host_vector = vm["host-list"].as<std::vector<std::string>>();
      LOG_INFO << "Node URIs:";
      for (auto i : options->host_vector) {
        LOG_INFO << "  " << i;
      }
    }

    if (vm.count("protobuf-endpoint")) {
      options->protobuf_endpoint = vm["protobuf-endpoint"].as<std::string>();
      LOG_INFO << "Protobuf Endpoint: " << options->protobuf_endpoint;
    } else {
      LOG_INFO << "Protobuf Endpoint was not set";
    }

    if (vm.count("working-dir")) {
      options->working_dir = vm["working-dir"].as<std::string>();
      LOG_INFO << "Working dir: " << options->working_dir;
    } else {
      LOG_INFO << "Working dir was not set.";
    }

    if (vm.count("inn-keys")) {
      options->inn_keys = vm["inn-keys"].as<std::string>();
      LOG_INFO << "INN keys file: " << options->inn_keys;
    } else {
      LOG_INFO << "INN keys file was not set.";
    }

    if (vm.count("node-keys")) {
      options->node_keys = vm["node-keys"].as<std::string>();
      LOG_INFO << "Node keys file: " << options->node_keys;
    } else {
      LOG_INFO << "Node keys file was not set.";
    }

    if (vm.count("key-pass")) {
      options->key_pass = vm["key-pass"].as<std::string>();
      LOG_INFO << "Key pass: " << options->key_pass;
    } else {
      LOG_INFO << "Key pass was not set.";
    }

    if (vm.count("stop-file")) {
      options->stop_file = vm["stop-file"].as<std::string>();
      LOG_INFO << "Stop file: " << options->stop_file;
    } else {
      LOG_INFO << "Stop file was not set. Use a signal to stop the node.";
    }
  }
  catch(std::exception& e) {
    LOG_ERROR << "error: " << e.what();
    return nullptr;
  }

  return options;
}

bool hasShard(std::string shard, const std::string& working_dir) {
  std::string shard_dir(working_dir+"/"+shard);
  fs::path dir_path(shard_dir);
  if (is_directory(dir_path)) return true;
  return false;
}

bool hasBlock(std::string shard, uint32_t block, const std::string& working_dir) {
  std::string block_path(working_dir+"/"+shard+"/"+std::to_string(block)+".blk");
  if (boost::filesystem::exists(block_path)) return true;
  return false;
}

uint32_t getHighestBlock(std::string shard, const std::string& working_dir) {
  std::string shard_dir(working_dir+"/"+shard);
  uint32_t highest = 0;
  for (auto i = boost::filesystem::directory_iterator(shard_dir);
      i != boost::filesystem::directory_iterator(); i++) {
    uint32_t some_block = std::stoul(i->path().stem().string(), nullptr, 10);
    if (some_block > highest) highest = some_block;
  }
  return highest;
}

std::vector<byte> ReadBlock(const std::string& shard, uint32_t block, const std::string& working_dir) {
  std::vector<byte> out;
  std::string block_path(working_dir+"/"+shard+"/"+std::to_string(block)+".blk");
  std::ifstream block_file(block_path, std::ios::in | std::ios::binary);
  block_file.unsetf(std::ios::skipws);

  std::streampos block_size;
  block_file.seekg(0, std::ios::end);
  block_size = block_file.tellg();
  block_file.seekg(0, std::ios::beg);
  out.reserve(block_size);

  out.insert(out.begin(),
             std::istream_iterator<byte>(block_file),
             std::istream_iterator<byte>());
  return out;
}

uint32_t SearchForTransaction(const std::string& shard, uint32_t start_block, const Signature& tx_id, const std::string& working_dir) {
  uint32_t highest = getHighestBlock(shard, working_dir);
  if (highest < start_block) return UINT32_MAX;

  std::vector<byte> target(tx_id.getRawSignature());
  for (uint32_t i=start_block; i<=highest; ++i) {
    std::vector<byte> block = ReadBlock(shard, i, working_dir);
    InputBuffer buffer(block);
    ChainState state;
    FinalBlock one_block(FinalBlock::Create(buffer, state));
    for (const auto& tx : one_block.getTransactions()) {
      if (tx->getSignature() == tx_id) {
        return i;
      }
    }
  }
  return UINT32_MAX;
}

std::vector<std::vector<byte>> SearchForAddress(const std::string& shard, uint32_t start_block, const Address& addr, const std::string& working_dir) {
  std::vector<std::vector<byte>> txs;
  uint32_t highest = getHighestBlock(shard, working_dir);
  if (highest < start_block) return txs;

  std::vector<byte> target(addr.getAddressRaw());
  for (uint32_t i=start_block; i<=highest; ++i) {
    std::vector<byte> block = ReadBlock(shard, i, working_dir);
    InputBuffer buffer(block);
    ChainState state;
    FinalBlock one_block(FinalBlock::Create(buffer, state));
    for (const auto& tx : one_block.getTransactions()) {
      for (const auto& xfer : tx->getTransfers()) {
        if (xfer->getAddress() == addr) {
          txs.push_back(tx->getCanonical());
        }
      } //end for xfers
    } //end for tx
  }
  return txs;
}

RepeaterResponsePtr HandleRepeaterRequest(const RepeaterRequestPtr& request, const std::string& working_dir) {
  RepeaterResponse response;
  try {
    response.request_timestamp = request->timestamp;
    response.operation = request->operation;
    DevvUri id = ParseDevvUri(request->uri);
    if (!id.valid) {
      response.return_code = 1052;
      response.message = "URI is not valid.";
      return std::make_unique<RepeaterResponse>(response);
    }
    if (!hasShard(id.shard, working_dir)) {
      response.return_code = 1050;
      response.message = "Shard "+id.shard+" is not available.";
      return std::make_unique<RepeaterResponse>(response);
    }
    if (id.block_height != UINT32_MAX) {
      if (!hasBlock(id.shard, id.block_height, working_dir)) {
        response.return_code = 1050;
        response.message = "Block "+std::to_string(id.block_height)+" is not available.";
        return std::make_unique<RepeaterResponse>(response);
      }
    }
    switch (request->operation) {
      case 1: {
        LOG_INFO << "Get Binary Block: "+request->uri;
        if (id.block_height == UINT32_MAX) {
          response.return_code = 1051;
          response.message = "Block height is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
		}
        response.raw_response = ReadBlock(id.shard, id.block_height, working_dir);
      }
        break;
      case 2: {
        LOG_INFO << "Get Binary Blocks Since: "+request->uri;
        uint32_t highest_block = getHighestBlock(id.shard, working_dir);
        if (id.block_height <= highest_block) {
          for (uint32_t i=id.block_height; i<=highest_block; ++i) {
            std::vector<byte> one_block = ReadBlock(id.shard, i, working_dir);
            response.raw_response.insert(response.raw_response.end()
                       , one_block.begin(), one_block.end());
          }
        }
      }
        break;
      case 3: {
        LOG_INFO << "Get Binary Chain: "+request->uri;
        uint32_t highest_block = getHighestBlock(id.shard, working_dir);
        for (uint32_t i=0; i<=highest_block; ++i) {
          std::vector<byte> one_block = ReadBlock(id.shard, i, working_dir);
          response.raw_response.insert(response.raw_response.end()
                     , one_block.begin(), one_block.end());
        }
      }
        break;
      case 4: {
        LOG_INFO << "Get Binary Transaction: "+request->uri;
        if (id.block_height == UINT32_MAX) {
          response.return_code = 1051;
          response.message = "Block height is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
		}
        if (id.sig.isNull()) {
          response.return_code = 1051;
          response.message = "Transaction signature is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
		}
        uint32_t height = SearchForTransaction(id.shard, id.block_height, id.sig, working_dir);
        std::vector<byte> block = ReadBlock(id.shard, height, working_dir);
        InputBuffer buffer(block);
        ChainState state;
        FinalBlock one_block(FinalBlock::Create(buffer, state));
        for (const auto& tx : one_block.getTransactions()) {
          if (tx->getSignature() == id.sig) {
            response.raw_response = tx->getCanonical();
            break;
		  }
        }
      }
        break;
      case 5: {
        LOG_INFO << "Get Protobuf Block: "+request->uri;
        std::vector<byte> block = ReadBlock(id.shard, id.block_height, working_dir);
        InputBuffer buffer(block);
        ChainState state;
        FinalBlock one_block(FinalBlock::Create(buffer, state));
        std::stringstream block_stream;
        Devv::proto::FinalBlock proto_block = SerializeFinalBlock(one_block);
        proto_block.SerializeToOstream(&block_stream);
        std::string proto_block_str = block_stream.str();
        std::vector<byte> pbuf_block(proto_block_str.begin(), proto_block_str.end());
        response.raw_response = pbuf_block;
        }
        break;
      case 6: {
        LOG_INFO << "Get Protobuf Blocks Since: "+request->uri;
        uint32_t highest_block = getHighestBlock(id.shard, working_dir);
        if (id.block_height <= highest_block) {
          for (uint32_t i=id.block_height; i<=highest_block; ++i) {
            std::vector<byte> block = ReadBlock(id.shard, i, working_dir);
            InputBuffer buffer(block);
            ChainState state;
            FinalBlock one_block(FinalBlock::Create(buffer, state));
            std::stringstream block_stream;
            Devv::proto::FinalBlock proto_block = SerializeFinalBlock(one_block);
            proto_block.SerializeToOstream(&block_stream);
            std::string proto_block_str = block_stream.str();
            std::vector<byte> pbuf_block(proto_block_str.begin(), proto_block_str.end());
            response.raw_response.insert(response.raw_response.end()
                       , pbuf_block.begin(), pbuf_block.end());
          }
        }
      }
        break;
      case 7: {
        LOG_INFO << "Get Protobuf Chain: "+request->uri;
        uint32_t highest_block = getHighestBlock(id.shard, working_dir);
        for (uint32_t i=0; i<=highest_block; ++i) {
          std::vector<byte> block = ReadBlock(id.shard, i, working_dir);
          InputBuffer buffer(block);
          ChainState state;
          FinalBlock one_block(FinalBlock::Create(buffer, state));
          std::stringstream block_stream;
          Devv::proto::FinalBlock proto_block = SerializeFinalBlock(one_block);
          proto_block.SerializeToOstream(&block_stream);
          std::string proto_block_str = block_stream.str();
          std::vector<byte> pbuf_block(proto_block_str.begin(), proto_block_str.end());
          response.raw_response.insert(response.raw_response.end()
                     , pbuf_block.begin(), pbuf_block.end());
        }
      }
        break;
      case 8: {
        LOG_INFO << "Get Protobuf Transaction: "+request->uri;
        if (id.block_height == UINT32_MAX) {
          response.return_code = 1051;
          response.message = "Block height is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
        }
        if (id.sig.isNull()) {
          response.return_code = 1051;
          response.message = "Transaction signature is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
        }
        uint32_t height = SearchForTransaction(id.shard, id.block_height, id.sig, working_dir);
        std::vector<byte> block = ReadBlock(id.shard, height, working_dir);
        InputBuffer buffer(block);
        ChainState state;
        FinalBlock one_block(FinalBlock::Create(buffer, state));
        for (const auto& tx : one_block.getTransactions()) {
          if (tx->getSignature() == id.sig) {
            InputBuffer buffer(tx->getCanonical());
            std::stringstream tx_stream;
            Devv::proto::Transaction pbuf_tx = SerializeTransaction(Tier2Transaction::QuickCreate(buffer));
            pbuf_tx.SerializeToOstream(&tx_stream);
            std::string proto_tx_str = tx_stream.str();
            std::vector<byte> tx_vector(proto_tx_str.begin(), proto_tx_str.end());
            response.raw_response = tx_vector;
            break;
		  }
        }
      }
        break;
      case 9: {
        LOG_INFO << "Check Transaction Since Block: "+request->uri;
        if (id.block_height == UINT32_MAX) {
          response.return_code = 1051;
          response.message = "Block height is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
        }
        if (id.sig.isNull()) {
          response.return_code = 1051;
          response.message = "Transaction signature is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
        }
        uint32_t block_height = SearchForTransaction(id.shard, id.block_height, id.sig, working_dir);
        if (block_height != UINT32_MAX) {
          Uint32ToBin(block_height, response.raw_response);
        } else {
          response.return_code = 3020;
          response.message = "Transaction not found since block "+std::to_string(id.block_height)+".";
          return std::make_unique<RepeaterResponse>(response);
		}
      }
        break;
      case 10: {
        LOG_INFO << "Get Transactions for Address Since Block: "+request->uri;
        if (id.block_height == UINT32_MAX) {
          response.return_code = 1051;
          response.message = "Block height is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
        }
        if (id.addr.isNull()) {
          response.return_code = 1051;
          response.message = "Transaction address is missing from the URI.";
          return std::make_unique<RepeaterResponse>(response);
        }
        std::vector<std::vector<byte>> txs = SearchForAddress(id.shard, id.block_height, id.addr, working_dir);
        std::stringstream pbuf_stream;
        Devv::proto::Envelope envelope = SerializeEnvelopeFromBinaryTransactions(txs);
        envelope.SerializeToOstream(&pbuf_stream);
        std::string proto_str = pbuf_stream.str();
        std::vector<byte> pbuf_vector(proto_str.begin(), proto_str.end());
        response.raw_response = pbuf_vector;
      }
        break;
      default:
        response.return_code = 1020;
        response.message = "Unknown operation: "+std::to_string(request->operation);
        return std::make_unique<RepeaterResponse>(response);
	}
	return std::make_unique<RepeaterResponse>(response);
  } catch (std::exception& e) {
    LOG_ERROR << "Repeater Error: "+std::string(e.what());
    response.return_code = 1010;
    response.message = "Repeater Error: "+std::string(e.what());
    return std::make_unique<RepeaterResponse>(response);
  }
}