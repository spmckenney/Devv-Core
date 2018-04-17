/**
 *
 */
#include "flatbuffers/devcash_primitives_generated.h"  // Already includes "flatbuffers/flatbuffers.h".

using namespace Devcash::fbs;

// Example how to use FlatBuffers to create and read binary buffers.

int main(int /*argc*/, const char * /*argv*/ []) {
  // Build up a serialized buffer algorithmically:
  flatbuffers::FlatBufferBuilder builder;

  flatbuffers::Offset<flatbuffers::Vector<int8_t>> addr(33);

  auto transfer = CreateTransfer(builder
                                 , addr
                                 , 1
                                 , 2
                                 , 0);

  flatbuffers::Offset<flatbuffers::Vector<int8_t>> nonce(20);
  flatbuffers::Offset<flatbuffers::Vector<int8_t>> sig(72);

  // Create a FlatBuffer Vector from an std::vector
  std::vector<flatbuffers::Offset<Transfer>> transfer_vector;
  transfer_vector.push_back(transfer);

  auto transfers = builder.CreateVector(transfer_vector);

  auto transaction = CreateTransaction(builder
                                       , transfers
                                       , nonce
                                       , sig);

  auto summary_item = CreateSummaryItem(builder
                                        , 1
                                        , 2);

  auto coin_map = CreateCoinMap(builder
                                , 12345
                                , summary_item);

  // Directly create FlatBuffer Vectors
  int8_t summary_addr_ar[33];
  auto summary_addr = builder.CreateVector(summary_addr_ar, 33);
  //flatbuffers::Offset<flatbuffers::Vector<int8_t>> summary_addr(33);

  std::vector<flatbuffers::Offset<CoinMap>> coinmap_vector;
  coinmap_vector.push_back(coin_map);
  auto coin_maps = builder.CreateVector(coinmap_vector);

  SummaryBuilder summary_builder(builder);
  summary_builder.add_addr(summary_addr);
  summary_builder.add_coin_maps(coin_maps);
  auto summary = summary_builder.Finish();

  auto validation_map = CreateValidationMap(builder
                                            , summary_addr
                                            , sig);

  std::vector<flatbuffers::Offset<ValidationMap>> validation_map_vector;
  validation_map_vector.push_back(validation_map);
  validation_map_vector.push_back(validation_map);
  auto validation_maps = builder.CreateVector(validation_map_vector);

  std::vector<flatbuffers::Offset<Transaction>> transaction_vector;
  auto transactions = builder.CreateVector(transaction_vector);

  std::vector<flatbuffers::Offset<Summary>> summary_vector;
  auto summaries = builder.CreateVector(summary_vector);

  std::vector<flatbuffers::Offset<Validation>> validation_vector;
  auto validations = builder.CreateVector(validation_vector);

  int64_t one{1}, two{2};
  auto int_128 = int128(one, two);
  auto int_256 = int256(int_128, int_128);

  auto ts_block = CreateTransactionSummaryBlock(builder
                                                , 10 // version
                                                , 100 // num_bytes
                                                , transactions
                                                , summaries);

  auto tsv_block = CreateTSVBlock(builder
                                  , &int_256 // hash_prev
                                  , &int_256 // hash_merkle_root
                                  , validations
                                  , 11 // time
                                  , ts_block);


  auto final_block = CreateFinalBlock(builder, tsv_block);

  builder.Finish(final_block);

  //auto final1 = GetFinalBlock(builder.GetBufferPointer());
  auto final1 = flatbuffers::GetRoot<FinalBlock>(builder.GetBufferPointer());

  auto s = final1->tsv_block()->validations()->size();

}
