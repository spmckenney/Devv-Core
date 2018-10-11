#!/usr/bin/env python
"""
Tools to send Devv transactions and proposals in Google Protobuf format.
"""

__copyright__ = "Copyright 2018, Devvio Inc"
__email__ = "security@devv.io"

import zmq
from . import devv_pb2 as dpb
import subprocess
import time
import tempfile

def create_devvsign_command(env_file, private_keyfile, key_pass):
    cmd = ["devv-sign"]
    cmd.extend(["--quiet-mode"])
    cmd.extend(["--envelope-file", env_file])
    cmd.extend(["--private-key", private_keyfile])
    cmd.extend(["--key-pass", key_pass])
    print(cmd)
    return cmd

class DevvTransfer(object):
    _address = bytes()
    _coin = 0
    _amount = 0
    _delay = 0

    def __init__(self, address=None, coin=None, amount=None, delay=None):
        print("Transfer: {}:{}:{}:{}".format(address, coin, amount, delay))

        if not coin:
            raise Exception("Coin type must be set")
        if not amount:
            raise Exception("Transfer amount must be set")

        self._address = bytes.fromhex(address)
        self._coin = int(coin)
        self._amount = int(amount)
        self._delay = int(delay)

    def get_pbuf(self):
        pb_tx = dpb.Transfer()
        pb_tx.address = self._address
        pb_tx.coin = self._coin
        pb_tx.amount = self._amount
        pb_tx.delay = self._delay
        return pb_tx


class DevvTransaction(object):
    _operation = dpb.UNKNOWN
    _nonce = bytes()
    _sig = bytes()
    _transfers = []

    def __init__(self, operation, nonce):
        self.set_operation(operation)
        self.set_nonce(nonce)

    def set_nonce(self, nonce):
        try:
            self._nonce = bytes.fromhex(nonce)
            print("Created nonce from hex number")
        except ValueError:
            self._nonce = nonce.encode("utf-8")
            print("Created nonce from string value")

    def set_operation(self, operation):
        op = operation.upper()
        if (op.find("CREATE") >= 0):
            self._operation = dpb.OP_CREATE
        elif(op.find("MODIFY") >= 0):
            self._operation = dpb.OP_MODIFY
        elif(op.find("EXCHANGE") >= 0):
            self._operation = dpb.OP_EXCHANGE
        elif(op.find("DELETE") >= 0):
            self._operation = dpb.OP_DELETE
        else:
            raise("Unknown operation")

    def add_transfer(self, address=None, coin=None, amount=None, delay=0, transfer_string=None):
        if (transfer_string):
            print("Adding transfer string")
            self.add_transfer_string(transfer_string)

        if (address):
            print("Adding transfer: {}:{}:{}:{}".format(address, coin, amount, delay))
            self._transfers.append(DevvTransfer(address=address, coin=coin, amount=amount, delay=delay))

    def add_transfer_string(self, transfer_string):
        p = transfer_string.split(":")
        if len(p) < 3:
            raise ValueError('Transfer string must contain "address:coin_type:amount[:delay]"')
        t = DevvTransfer(address=p[0], coin=p[1], amount=p[2], delay=p[3] if len(p) == 4 else 0)
        self._transfers.append(t)

    def get_pbuf(self):
        pb_tx = dpb.Transaction()

        pb_transfers = []
        for transfer in self._transfers:
            pb_transfers.append(transfer.get_pbuf())
        pb_tx.xfers.extend(pb_transfers)

        pb_tx.operation = self._operation
        pb_tx.nonce = self._nonce
        pb_tx.sig = self._sig

        return pb_tx


def get_envelope(tx):
    pbtx = tx.get_pbuf()

    print("pbuf")
    print(pbtx)

    env = dpb.Envelope()
    env.txs.extend([pbtx])
    return env


def send_envelope(env, uri):
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect(uri)

    print("Sending message in 1")
    time.sleep(1)
    socket.send(env.SerializeToString())
    time.sleep(1)
    x = socket.recv()
    print("Sent message")


class EnvFile:
    def __init__(self, env, tmp_dir='/tmp'):
        self._env_file = tempfile.NamedTemporaryFile(dir=tmp_dir, suffix="_env.pbuf")

        self._estr = env.SerializeToString()
        print("estr: ", len(self._estr))
        self._env_file.write(env.SerializeToString())
        self._env_file.flush()

    def __del__(self):
        self._env_file.close()

    def filename(self):
        return self._env_file.name


class KeyFile:
    def __init__(self, address, key, tmp_dir='/tmp'):
        self._tmp_dir = tmp_dir
        lsize = 64
        self._key = [address]
        self._key.append('-----BEGIN ENCRYPTED PRIVATE KEY-----')
        self._key.extend([key[i:i+lsize] for i in range(0, len(key), lsize) ])
        self._key.append('-----END ENCRYPTED PRIVATE KEY-----')

        self._key_file = None
        self.write()

    def __del__(self):
        if self._key_file:
            self._key_file.close()
            self._key_file = None

    def write(self):
        self._key_file = tempfile.NamedTemporaryFile(dir=self._tmp_dir, suffix=".devvkey", mode='w+', delete=False)
        for i in self._key:
            self._key_file.write(i)
            self._key_file.write('\n')
        self._key_file.close()

    def filename(self):
        return self._key_file.name

    def display(self):
        for i in self._key:
            print("{}".format(i))

