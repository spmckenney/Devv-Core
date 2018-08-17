package io.devv.test;

import org.zeromq.ZMQ;

import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;

import proto.Devv.Devv.Envelope;
import proto.Devv.Devv.Transaction;
import proto.Devv.Devv.Transfer;

public class DevvTestMain {

	static final public String ADDR_1 = "2102514038DA1905561BF9043269B8515C1E7C4E79B011291B4CBED5B18DAECB71E4";
	static final public String ADDR_1_KEY = "-----BEGIN ENCRYPTED PRIVATE KEY-----\\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAh8ZjsVLSSMlgICCAAw\\nHQYJYIZIAWUDBAECBBBcQGPzeImxYCj108A3DM8CBIGQjDAPienFMoBGwid4R5BL\\nwptYiJbxAfG7EtQ0SIXqks8oBIDre0n7wd+nq3NRecDANwSzCdyC3IeCdKx87eEf\\nkspgo8cjNlEKUVVg9NR2wbVp5+UClmQH7LCsZB5HAxF4ijHaSDNe5hD6gOZqpXi3\\nf5eNexJ2fH+OqKd5T9kytJyoK3MAXFS9ckt5JxRlp6bf\\n-----END ENCRYPTED PRIVATE KEY-----";
	static final public String ADDR_2 = "2102E14466DC0E5A3E6EBBEAB5DD24ABE950E44EF2BEB509A5FD113460414A6EFAB4";
	static final public String SOME_NONCE_HEXSTR = "00112233445566778899AABBCCDDEEFF";
	static final public String ANNOUNCER_URL = "tcp://*:55706";
	
	
	static public byte[] hexStringToByteArray(String s) {
	    int len = s.length();
	    byte[] data = new byte[len / 2];
	    for (int i = 0; i < len; i += 2) {
	        data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
	                             + Character.digit(s.charAt(i+1), 16));
	    }
	    return data;
	}
	
	public Transaction getTransaction(ByteString sender, ByteString receiver, long coin, long amount, long delay, byte[] nonce, byte[] sig) {
		Transfer xfer = Transfer.newBuilder()
				.setAddress(sender)
				.setAmount(-1)
				.setCoin(0)
				.setDelay(0)
				.build();
		Transfer xfer2 = Transfer.newBuilder()
				.setAddress(receiver)
				.setAmount(1)
				.setCoin(0)
				.setDelay(0)
				.build();
		Transaction tx = Transaction.newBuilder()
				.setNonceSize(sender.size())
				.setXferSize(2)
				.addXfers(xfer)
				.addXfers(xfer2)
				.setOperationValue(2)
				.setNonce(ByteString.copyFrom(nonce))
				.setSig(ByteString.copyFrom(sig))
				.build();
		return tx;
	}
	
	public native byte[] SignTransaction(byte[] tx, byte[] privateKey);
	
	public static void main(String[] args) {
		try {
			System.loadLibrary("Devv");
			
			ZMQ.Context context = ZMQ.context(1);
	
	        ZMQ.Socket requester = context.socket(ZMQ.REQ);
	        requester.connect(ANNOUNCER_URL);
			
			DevvTestMain test = new DevvTestMain();
			Transaction tx = test.getTransaction(ByteString.copyFrom(hexStringToByteArray(ADDR_1)),
					ByteString.copyFrom(hexStringToByteArray(ADDR_2)),
					0L, 1L, 0L,
					hexStringToByteArray(SOME_NONCE_HEXSTR),
					hexStringToByteArray(ADDR_1));
			byte[] signed = test.SignTransaction(tx.toByteArray(), hexStringToByteArray(ADDR_1_KEY));
			try {
			  Transaction tx2 = Transaction.parseFrom(signed);
			  Envelope env = Envelope.newBuilder().addTxs(tx2).build();
			  requester.send(env.toByteArray(), 0);
			  byte[] reply = requester.recv(0);
	          System.out.println("Received " + new String(reply));
	
			} catch (InvalidProtocolBufferException ipbe) {
				System.err.println("InvalidProtocolBufferException: "+ipbe.getMessage());
			}
	        requester.close();
	        context.term();
		} catch (Exception e) {
			System.err.println(e.getClass()+": "+e.getMessage());
		}
        
	}
	
}
