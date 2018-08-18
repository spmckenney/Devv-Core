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

//Consensus Params
static const int kACTIVATION_ROUNDS = 334;
static const unsigned int kPROPOSAL_TIMEOUT = 60000;
static const int kVALIDATION_PERCENT = 51;

static const unsigned int kSYNC_PORT_BASE = 55330;

enum eDebugMode {off, on, toy, perf};

struct DevcashContext {

  /**
   * Constructor
   */
  DevcashContext(unsigned int current_node, unsigned int current_shard
    , eAppMode mode
    , const std::string& inn_key_path
    , const std::string& node_key_path
    , const std::string& key_pass
    , unsigned int batch_size=10000
    , unsigned int max_wait=0)
    : current_node_(current_node)
    , current_shard_(current_shard)
    , app_mode_(mode)
    , uri_(get_uri_from_index(current_node+current_shard*peer_count_))
    , inn_keys_(inn_key_path)
    , node_keys_(node_key_path)
    , key_pass_(key_pass)
    , batch_size_(batch_size)
    , max_wait_(max_wait)
  {
  }

  const std::string kINN_KEY = "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIIBDjBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQIGT9wNeMNNvcCAggAMB0GCWCGSAFlAwQBAgQQnleRZC6seC/5+gF5W91OTwSBwAjNeW4dkUbohXhuuy8BGlx7wV/AtBQeMwrVCG/eXj8H9mG/P2MfPf1kbYa18qFOiw46Ih7HhP9QaFVBoNHtsv8/epwAw5stFKIFO/AIDu0piqBqy9KpAEVKTbU2uTqCgGZKRnHCAmE9jgnJoboE41ySWmr7P63tq4LVNIwtPI0xSHxn9fBe1i7BaIQV3fUhGlPW7bgMODoVe7tysCldM/l1O1iDX/eodHOTv0RQjraNeZVqzsPBrSOv7jAgjS0AGA==\n-----END ENCRYPTED PRIVATE KEY-----";

  const std::string kINN_ADDR = "0272B05D9A8CF6E1565B965A5CCE6FF88ABD0C250BC17AB23745D512095C2AFCDB3640A2CBA7665F0FAADC26B96E8B8A9D";

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
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIIBDjBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQIZjcCo0ODrskCAggAMB0GCWCGSAFlAwQBAgQQN/J3yTpQ7unPZGT9DKJlmgSBwLXr2Pp4GSwXacelxUk3KFWVUYfcsQGz3JYO3GTdoSyLuWjsRpRA82PSQaP1Vn5C48ZZJlXAwxRn547eBu5WZ2QMGiAwNIzmFKqYI9pYiWsM13F82uDmFGkjbt2HSRx6tIhuA9A/40PIqjVjdkjQkEdiFxh/cjytd4kovH9vee17nSQ29m39AJdFAbukWSak2C5VKvysn1BItZE3qzZVITIV4IBEGRiOjXZcacXKxdu3rjsF9XkD/dV1x/qMLTuycA==\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIIBDjBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQIb3S5HoFxfvYCAggAMB0GCWCGSAFlAwQBAgQQr21QQhxXn9VGTgyehabg3QSBwE6NAVP/zFWXKUVVE0daHgj81vgfiOJAvi1ayQPzaGv26alNAIu22ntLK+SWcKrqEIrdqCzOT3SoIs8XtEUibpj14ZOp7UzhGQnPsme7oFX2caCVQh76pNfPzJkMoEH2YX/m0HyNBrsdBkWORiPRdfgiv/gMkBIN8ai2jdmWzBmlOX+coQ3hMG5p3rSoQPxBm5vPONQIaM9DlZrwkMq6fXCz0HZwdCLiqKJs0Xl1V4WCXmtXslxeyTPZq/c2YucVPg==\n-----END ENCRYPTED PRIVATE KEY-----",
      "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIIBDjBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQIC3emoeSyM8MCAggAMB0GCWCGSAFlAwQBAgQQ8DCivDaU4M6hX2WLrHN41ASBwCtuSZ/ybssOKRxe3a72EcqhKvdpWaT/x6AGIOzdakpAnX24YOEadUhgoRlZjHmQ0jMKezOXgcoaOJ9y7+zxyfoBJZnKAKiJhn4Jz5pdQW17riHucsVqOvHHhO+3UtwzpPrrulcjZRn29AwN6HeuvjvZRxbn16Isst97Sal1EADNw5Hri/MJ5BHQWe6LRJbTKv3llP6mAgjuwyZECInjBMZLjfFZLtF1lrviHCFR9ZA/QLoQti4fjdzz1Lurpmf0zw==\n-----END ENCRYPTED PRIVATE KEY-----"
  };

  const std::vector<std::string> kNODE_ADDRs = {
      "03EFE5DA5D03078E91FFF5FA5F786190F53F77D7CBA3FFB7DA1F11D50222403DBD9ED299C91ABC5C0AC4367BF260A4083E",
      "0286C1B1A066728FAABA4D9C795846FF9910C44BD54386F05E088F409BFE56906D41AD60FD1C982920C9017A799E7F9DFF",
      "02D6663B8C28C80D977C8943B9D9C6441E1C2D08FBA43131016CCCB11B358B531A3379E1289206E13F75B50E08274BB719"
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
  std::string get_key_password() const { return key_pass_; }
  unsigned int get_batch_size() const {return batch_size_; }
  unsigned int get_max_wait() const {return max_wait_; }

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

  //AES private key password
  std::string key_pass_;

  unsigned int batch_size_;
  unsigned int max_wait_;
};

} /* namespace Devcash */

#endif /* COMMON_DEVCASH_CONTEXT_H_ */
