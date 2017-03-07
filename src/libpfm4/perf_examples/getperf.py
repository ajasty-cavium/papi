#!/usr/bin/python

import os
import platform
import sys
import binascii
from socket import *
import struct

def recv_bytes (sock, n):
    data = ''
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if (chunk == ''):
            break
        data += chunk
    return data

def sendQuery (sock, query):
    packer = struct.Struct('I')
    unpacker = struct.Struct('I')
    pd = packer.pack(len(query.encode('utf-8')))
    sock.send(pd)
    sock.send(query)
    data = recv_bytes(sock, 4)
    slen = struct.unpack('I', data)[0]
    values = sock.recv(int(slen))
    k = values.split()
    return k

def main ():
    sock = socket(AF_INET, SOCK_STREAM)
    server_address = (sys.argv[1], 9999)

    query_toplev = "CPU_CYCLES,INST_RETIRED,STALLED_CYCLES_FRONTEND,STALLED_CYCLES_BACKEND\0"
    query_br = "BRANCH_MISPRED,BRANCH_PRED"
    sock = socket(AF_INET, SOCK_STREAM)
    sock.connect(server_address)

    try:
        while True:
            k = sendQuery(sock, query_toplev)
            ipc = float(k[1]) / float(k[0]) * 100.0
            fes = float(k[2]) / float(k[0]) * 100.0
            bes = float(k[3]) / float(k[0]) * 100.0
            out = format("IPC=%2.2f - FRONTEND=%2.2f - BACKEND=%2.2f. " % (ipc, fes, bes))
            sys.stdout.write(out)
            k = sendQuery(sock, query_br)
            brpred = (float(k[0]) / float(k[1])) / 100.0
            out = format("Branch mispred rate=%2.2f.\r" % brpred)
            sys.stdout.write(out)
            sys.stdout.flush()

    finally:
        sock.close()

if __name__ == "__main__":
    main()

