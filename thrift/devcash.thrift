namespace Devcash.thrift.primitives

struct DCTransferStruct {
  1: string addr_,
  2: i64 amount_,
  3: i16 coinIndex_,
  4: i64 delay_,
}

struct DCTransactionStruct {
  1: i32 nonce_,
  2: list<DCTransferStruct> xfers_,
  3: string sig_,
}

struct DCSummaryStruct {
  1: map<string, map<i16, map<i64, i64>>> summary_map_,
}

struct DCValidationStruct {
  1: map<string, string> validations_,
}

struct ChainStateStruct {
  1: map<string, list<long>> chain_state_,
}

struct DCBlockStruct {
  1: i16 nVersion_,
  2: i256 hashPrevBlock_,
  3: i256 hashMerkleRoot_,
  4: i32 nBytes_,
  5: i64 nTime_,
  6: i32 txSize_,
  7: i32 sumSize_,
  8: i32 vSize_,
  9: list<DCTransactionStruct>,
  10: list<DCSummaryStruct>,
  11: list<DCValidationStruct>,
}

struct BlockChain {
  1: list<DCBlockStruct> chain_,
  2: ChainStateStruct current_state_,
}
