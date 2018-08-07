#!/bin/python

import sys
sys.path.append(".")

import zmq
import time
import devv_pb2

tx = devv_pb2.Transaction()

tsfrs = []

tsfr = devv_pb2.Transfer()
tsfr.address = bytes.fromhex("310272B05D9A8CF6E1565B965A5CCE6FF88ABD0C250BC17AB23745D512095C2AFCDB3640A2CBA7665F0FAADC26B96E8B8A9D")
tsfr.coin = 0
tsfr.amount = -6
tsfr.delay  = 0
tsfrs.append(tsfr)

tsfr = devv_pb2.Transfer()
tsfr.address = bytes.fromhex("2102E14466DC0E5A3E6EBBEAB5DD24ABE950E44EF2BEB509A5FD113460414A6EFAB4")
tsfr.coin = 0
tsfr.amount = 2
tsfr.delay  = 0
tsfrs.append(tsfr)

tsfr = devv_pb2.Transfer()
tsfr.address = bytes.fromhex("2102C85725EE136128BC7D9D609C0DD5B7370A8A02AB6F623BBFD504C6C0FF5D9368")
tsfr.coin = 0
tsfr.amount = 2
tsfr.delay  = 0
tsfrs.append(tsfr)

tsfr = devv_pb2.Transfer()
tsfr.address = bytes.fromhex("21030DD418BF0527D42251DF0254A139084611DFC6A0417CCE10550857DB7B59A3F6")
tsfr.coin = 0
tsfr.amount = 2
tsfr.delay  = 0
tsfrs.append(tsfr)

tx.xfers.extend(tsfrs)
tx.operation = devv_pb2.OP_CREATE
tx.nonce = bytes.fromhex("86525F0665010000")
tx.sig = bytes.fromhex("69306502302AABAA749F88403C904C6939BDE366992A43282A63B0A78B1F687188C44F27E7DCC8FC8D873ED8D7A87425FCCE2C7384023100BE4A47EA1CF0EFD8E9FC98A619D441EA87BCA7FE6B323BB56B1D8760DE3ADBAC070105FBAA6BE22411B80C0E77FE0D010000")

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

print("Sending message in 3")
time.sleep(3)

socket.send(tx.SerializeToString())

print("Done")

time.sleep(1)
