#include "io_devv_test_DevvTestMain.h"
#include "devv.pb.h"
#include "devv_pbuf.h"

JNIEXPORT jbyteArray JNICALL Java_io_devv_test_DevvTestMain_SignTransaction
  (JNIEnv* env, jobject obj, jbyteArray proto_tx, jbyteArray private_key) {
    std::string pbuf_str;
    jsize tx_pbuf_len = env->GetArrayLength(env, proto_tx);
    jint* tx_pbuf_body = env->GetIntArrayElements(env, proto_tx, 0);
    for (int i=0; i<tx_pbuf_len; i++) {
      pbuf_str.push_back(tx_pbuf_body[i]);
    }
    Devv::proto::Transaction tx;
    tx.ParseFromString(pbuf_str);

    std::string key_str;
    jsize key_pbuf_len = env->GetArrayLength(env, private_key);
    jint* key_pbuf_body = env->GetIntArrayElements(env, private_key, 0);
    for (int i=0; i<key_pbuf_len; i++) {
      key_str.push_back(key_pbuf_body[i]);
    }

    Tier2TransactionPtr t2tx_ptr = CreateTransaction(tx, key_str);
    Devv::proto::Transaction tx;
    for (const std::vector<TransferPtr>& xfer_ptr : t2tx_ptr->getTransfers()) {
      Devv::proto::Transfer* xfer = tx->add_xfers();
      xfer.set_address(Bin2Str(xfer_ptr->getAddress()));
      xfer.set_coin(xfer_ptr->getCoin());
      xfer.set_amount(xfer_ptr->getAmount());
      xfer.set_delay(xfer_ptr->getDelay());
    }
    tx->set_nonce_size(t2tx_ptr->getNonce().size());
    tx->set_xfer_size(t2tx_ptr->getTransfers().size());
    tx->set_nonce(Bin2Str(t2tx_ptr->getNonce()));
    tx->set_sig(Bin2Str(t2tx_ptr->getSignature()));
    tx->set_operation(t2tx_ptr->getOperation());

    size_t final_tx_len = tx->ByteSizeLong();
    google::protobuf::uint8* bin_ptr = InternalSerializeWithCachedSizesToArray();

    jbyteArray ret = env->NewByteArray(final_tx_len);
    env->SetByteArrayRegion(ret, 0, final_tx_len, (jbyte*) bin_ptr);

    env->ReleaseIntArrayElements(proto_tx, tx_pbuf_body, 0);
    env->ReleaseIntArrayElements(private_key, key_pbuf_body, 0);
    return ret;
  }