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

def main ():
    sock = socket(AF_INET, SOCK_STREAM)
    server_address = (sys.argv[1], 9999)
    sock.connect(server_address)

    #query = "CPU_CYCLES\0"
    query = "CPU_CYCLES,INST_RETIRED,STALLED_CYCLES_FRONTEND,STALLED_CYCLES_BACKEND\0"
    packer = struct.Struct('I')
    unpacker = struct.Struct('I')
    pd = packer.pack(len(query.encode('utf-8')))

    try:
        print pd
        sock.send(pd)
        print query
        sock.send(query)
        data = recv_bytes(sock, 4)
        slen = struct.unpack('I', data)[0]
        query = sock.recv(int(slen))
        k = query.split()
        ipc = float(k[1]) / float(k[0])
        print(query)
        print(ipc)

    finally:
        sock.close()

if __name__ == "__main__":
    main()
