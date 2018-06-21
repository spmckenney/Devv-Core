/*
 * devcashcontext.h runtime constants for this node
 *
 *  Created on: Mar 20, 2018
 *      Author: Nick Williams
 */

#ifndef COMMON_DEVCASH_CONTEXT_H_
#define COMMON_DEVCASH_CONTEXT_H_

#include <string>
#include <vector>
#include <sys/stat.h>

#include "common/devcash_types.h"

namespace Devcash {

static const int kDEFAULT_WORKERS = 128;
static const int kVALIDATOR_THREADS = 10;
static const int kCONSENSUS_THREADS = 10;
//millis of sleep between main shut down checks
static const int kMAIN_WAIT_INTERVAL = 100;

//Consensus Params
static const int kACTIVATION_ROUNDS = 334;
static const unsigned int kPROPOSAL_TIMEOUT = 60000;
static const int kVALIDATION_PERCENT = 51;

static const unsigned int kSYNC_PORT_BASE = 55330;

inline bool exists_test1(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

inline bool exists_test(const std::string &name) {
  if (FILE *file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  } else {
    return false;
  }
}

struct DevcashContext {

  /**
   * Constructor
   */
  DevcashContext(unsigned int current_node, unsigned int current_shard
    , eAppMode mode
    , const std::string& inn_key_path
    , const std::string& node_key_path
    , const std::string& wallet_key_path
    , unsigned int sync_port
    , const std::string& sync_host = "")
    : current_node_(current_node)
    , current_shard_(current_shard)
    , app_mode_(mode)
    , uri_(get_uri_from_index(current_node+current_shard*peer_count_))
    , inn_keys_(inn_key_path)
    , node_keys_(node_key_path)
    , wallet_keys_(wallet_key_path)
    , sync_host_(sync_host)
    , sync_port_(sync_port)
  {}

  const std::string kINN_KEY = "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAgBcpJHkg56mAICCAAw\nHQYJYIZIAWUDBAECBBCHa2RxQu9uIGCnJXiJjMF2BIGQcnO7UeEAHFauiaheEQPW\nn5cgO1sAlY7r3kMWgX4d5qu0DnEVzNN6F4RkQDbyvWwS1YHzdVn17oynnqtL9RS6\nqYrt1xhFFwp6Z+R/uqSk+3xZgMSYf2wpUJ9pqhm0JBTqOelZ37yF57+585ez4ujD\nA1gnH1w36y5hnZqRWVvi3eRXxCr5wqF8dNwFuxLpAuse\n-----END ENCRYPTED PRIVATE KEY-----";

  const std::string kINN_ADDR = "0242DC4F1D89513CBA236C895B117BC7D0ABD6DC8336E202D93FB266E582C79624";

  const std::vector<std::string> kADDRs = {
      "02514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4",
      "035C0841F8F62271F3058F37B32193360322BBF0C4E85E00F07BCB10492E91A2BD",
      "02AF4E6872BECE20FA31F181E891D60C7CE49F95D5EF9448F8B7F7E8D621AE008C",
      "03B17F124E174A4055128B907401CAEECF6D5BC56A7DF6201381A8B939CAC12721",
      "025054C2E2C8878B5F0573332CD6556BEB3962E723E35C77582125D766DCBDA567",
      "022977B063FF1AA08758B0F5CBDA720CBC055DFE991741C4F01F110ECAB5789D03"
  };

  const std::vector<std::string> kADDR_KEYs = {
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAh8ZjsVLSSMlgICCAAw\nHQYJYIZIAWUDBAECBBBcQGPzeImxYCj108A3DM8CBIGQjDAPienFMoBGwid4R5BL\nwptYiJbxAfG7EtQ0SIXqks8oBIDre0n7wd+nq3NRecDANwSzCdyC3IeCdKx87eEf\nkspgo8cjNlEKUVVg9NR2wbVp5+UClmQH7LCsZB5HAxF4ijHaSDNe5hD6gOZqpXi3\nf5eNexJ2fH+OqKd5T9kytJyoK3MAXFS9ckt5JxRlp6bf\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAj26KaMMBcx8gICCAAw\nHQYJYIZIAWUDBAECBBCAi3UqcRVw4celU/EfTYlwBIGQNWD5TjggnHK2yqDOq9uh\nRhQ9ZYWecLKFzirki+/nLdlk+MBIGQIZ/Ra4LStBc6pu0hv6HAEzjt9hhOAh2dLG\n28kieREpPY1vvNpfv3w9cQ5aH0iFoQXCxjwluGOpUBEmc212U56XivOE5DGchx81\nKxLIpWF4c8Jb3dwBdL4GuKCbt+x7AK+Hjj6XnUAhiNGN\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAhzjMEUwJByZwICCAAw\nHQYJYIZIAWUDBAECBBAFlRRNbCc7mjdARZH4AgWKBIGQ9plIbYr2WbEMY4N/bV+j\n+uI0k0hn68jzZjmzDIesAjRsqJ6EsKnwDb93fW85iA/wF44K4fgn8Y8fAfAUymdQ\nMtXnSbSuteOGfuYvVnusL211UQkg8Al34pTB1igTkEWh3BpTp2/Qf1CpLIFiOeMq\nNcMaALuG2w4gkJaHS33AS2GvHCr6FA8Q7rKNrFNt3HzB\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAihcpS7r/V0BAICCAAw\nHQYJYIZIAWUDBAECBBACED7jSSDz9pnEViP6WpN/BIGQUQHkt+twjgpGr8LRWQuW\niAiFT7dWMGUIAS1esRkJytVPwQDuYiSf406kqRHWoH5ZeWK0Kk2VvEoVuPlvA1nW\nJhYRnlA1R2IL3I4OftT6CORBF1i8EBM3Ah+bUbcqh4VJUw+weABLXq/0+k1tVA8e\nsxqcUZVQk/n7g6kuoeZgi8gSCrEXEby67MTnkvjlfkpy\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAh1BrpIaOV33AICCAAw\nHQYJYIZIAWUDBAECBBDJH6sU+1WQnLPNGK5Dwz1DBIGQg08MOyvhVV3UohqZ5Agy\nNNX7TMdmWC1rel35ur6GWM77Cl4uZvT4P56hinbrrtH4En5VUQTF70JVaCyzf5X9\nqGXKxOyFez4mWEK9gQWsYfFQSsjU+DW6Z6wb7pscwFQ4zL4o2+61M8PTTMm6WoyD\nd+9aeJvaUSIvYTWFkiHPuz52Uhyguyp+qcad/yokx7eI\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAg/scbUqllDxgICCAAw\nHQYJYIZIAWUDBAECBBAPtIhz2lR4u4SoxGiwuEJnBIGQr0cLlguY/h/juBrESEq4\nk6TuOSyqg64tr7E/wRclcIP3CHpnQBvlyd2M3Rq2WCGOObhrJ7MvvoSk4Bm/evWO\nbgDW7HqC84OdNr49nJe/+1RyS8OglA+TFQi0HEQIHn+ePtygDpIdeyKGdXZkqbaN\npbioudfHPQ5Lv2CsEBmSvR6Eb/DyxOuz7JZtoSd55T3F\n-----END ENCRYPTED PRIVATE KEY-----"
  };

  const std::vector<std::string> kNODE_KEYs = {
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAhNiEofOPlL+AICCAAw\nHQYJYIZIAWUDBAECBBD1Og0jylm8xZp8YhvPePIaBIGQq8W9dHr+rmuwGQ3Jl324\nq3Bkk7JVFWutewCV4kQ1+h5ZdjMulglCLzC52IIGp7ZhesPys4WtVRA/SH0MwaIy\nAe6fcONjAIXJhy+AbG9HQ4xQLMwy76bFOJowkPN3Lk6xAfgIzdwliX8IA/HbG+PI\nqmw/unWjEslvadDLSuPZLRXKaUeMvttI7oZ7Sf7+O46d\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAgHxfK/19g1OwICCAAw\nHQYJYIZIAWUDBAECBBBPOPI3ivjR3QUlnYaXF98TBIGQySq++0wFbYEAltbr2f3L\neovYXbBPWvXhL8+lFt1McSmVKaYIH014/7oJlz7nSBjUB5X0Y95iweQfDfCdSABK\nT4s46LvlcBd3Rq2GatHiCMuC3P/Q1du7dRBkQih5/Y32E3w2A+RIyg0+kyK8bfHM\nZyoQpAkTcYqzNnLfT4cd0b/EGu6oRQyk9YFYW5g+u+tF\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAjGudWSMspaRwICCAAw\nHQYJYIZIAWUDBAECBBAa34VLNuJcsmZ6G0KvvusFBIGQY/uYVOM9zTs98p4N29qg\nI6XBprHCu+qGeEXGIEgXBSK6iDk6hW3BCnz5T74neqkDzPJK+t+bXDCRzu4eKU47\ncpPYtz04w5NLXAJjqHEXQA+uW7CIA+9NireczsLO2hfx/PsA3/MTOehzYxGKy2gu\nxOnxfeOwNioI3TPbVdZ/mw2T/7WdRIJeEqErb2Wt09Np\n-----END ENCRYPTED PRIVATE KEY-----"
  };

  const std::vector<std::string> kNODE_ADDRs = {
      "04158913328E4469124B33A5D421665A36891B7BCB8183A22CC3D78239A89073FEB7432E6477663CDE2E032A56687617800B97FC0EBD9F3AC30F683B6C4A89D1D8",
      "0462CAF2CC08A7763A7F7B51590D016499079116E37892195E2AC8DE2DA54834D346558C56EE496104A4B533507948CEC5D8128AD2EDAE63BA0DC29F5D1D5AA5F3",
      "04B14F28DA8C0389BC385BA3865DB3FC7FAFA8FA4715C0ADAADAC52F2EB3E7FDCD695B439F9ACDCC90E55C1F9C48D7EB5B3BFD6C64EC89B1A6108F4B1B01A3FCA4"
  };

  size_t get_peer_count() const { return peer_count_; }
  unsigned int  get_current_node() const { return current_node_+current_shard_*peer_count_; }
  unsigned int  get_current_shard() const { return current_shard_; }
  eAppMode get_app_mode() const { return app_mode_; }
  std::string get_uri() const { return uri_; }

  std::string get_uri_from_index(unsigned int node_index) const {
    return (uri_prefix_ + std::to_string(node_index));
  }

  std::string get_shard_uri() const {
    return ("shard-" + std::to_string(current_shard_));
  }

  std::string get_inn_key_path() const { return inn_keys_; }
  std::string get_node_key_path() const { return node_keys_; }
  std::string get_wallet_key_path() const { return wallet_keys_; }

  std::string get_sync_host() const { return (sync_host_+":"+std::to_string(sync_port_)); }

private:
  /** Number of connected peers */
  const size_t peer_count_ = 3;

  // Node index of this process
  unsigned int current_node_;

  // Shard Index of this process
  unsigned int current_shard_;

  // Process mode
  eAppMode app_mode_ = scan;

  // Prefix for devcash uris
  std::string uri_prefix_ = "RemoteURI-";

  // This processes uri
  std::string uri_;

  //key file paths
  std::string inn_keys_;
  std::string node_keys_;
  std::string wallet_keys_;

  // Host the nodes will sync to
  std::string sync_host_;

  // Port the nodes will sync to
  unsigned int sync_port_;
};

} /* namespace Devcash */

#endif /* COMMON_DEVCASH_CONTEXT_H_ */
