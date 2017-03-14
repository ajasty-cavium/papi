#!/usr/bin/python

import os
import platform
import sys
import binascii
from socket import *
import struct
import curses

def recv_bytes (sock, n):
    #unpacker = struct.Struct('I')
    data = ''
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if (chunk == ''):
            break
        data += chunk
    slen = struct.unpack('I', data)[0]
    return slen

def write_int(sock, n):
    packer = struct.Struct('i')
    pd = packer.pack(int(n))
    sock.send(pd)
    return

def sendQuery (sock, query):
    write_int(sock, len(query.encode('utf-8')))
    sock.send(query)
    slen = recv_bytes(sock, 4)
    values = sock.recv(int(slen))
    k = values.split()
    return k

def do_arm64_toplev(sock, raw):
    query_toplev = "CPU_CYCLES,INST_RETIRED,STALLED_CYCLES_FRONTEND,STALLED_CYCLES_BACKEND\0"
    query_br = "BRANCH_MISPRED,BRANCH_PRED,L1I_CACHE_REFILL,L1I_CACHE_ACCESS,L1D_CACHE_REFILL,L1D_CACHE_ACCESS\0"
    try:
            k = sendQuery(sock, query_toplev)
            ipc = float(k[1]) / float(k[0])
            fes = float(k[2]) / float(k[0]) * 100.0
            bes = float(k[3]) / float(k[0]) * 100.0
            if raw == True:
                out = format("%2.2f,%2.2f,%2.2f," % (ipc, fes, bes))
            else:
                out = format("IPC=%2.2f - FRONTEND=%2.2f%% - BACKEND=%2.2f%%. " % (ipc, fes, bes))
            k = sendQuery(sock, query_br)
            brpred = (float(k[0]) / float(k[1])) * 100.0
            dmiss = (float(k[2]) / float(k[3])) * 100.0
            imiss = (float(k[4]) / float(k[5])) * 100.0
            if raw == True:
                out += format("%2.2f,%2.2f,%2.2f," % (brpred, imiss, dmiss))
            else:
                out += format("Branch mispred rate=%2.2f%%, L1I miss=%2.2f%%, L1D miss=%2.2f%%." % (brpred, imiss, dmiss))
            return out
    finally:
        sys.stdout.flush()

def do_arm64_pipeline(sock, raw):
    query_pipe_fe = "CPU_CYCLES,MAP_ISB_EMPTY,MAP_ISB_FULL,MAP_ISSQ_RECYCLE,MAP_ROB_RECYCLE,MAP_BUB_RECYCLE\0"
    query_pipe_be_= "MAP_GPR_RECYCLE,MAP_LRQ_RECYCLE,MAP_SRQ_RECYCLE,MAP_BSR_RECYCLE,MAP_BRID_RECYCLE\0"
    try:
            k = sendQuery(sock, query_pipe_fe)
            if raw == True:
                out = format("%i,$i,$i,$i,$i,$i,$i," % (k[0], k[1], k[2], k[3], k[4], k[5], k[6]))
            else:
                out = format("IPC=%2.2f - FRONTEND=%2.2f%% - BACKEND=%2.2f%%. " % (ipc, fes, bes))
            k = sendQuery(sock, query_pipe_be)
            brpred = (float(k[0]) / float(k[1])) * 100.0
            dmiss = (float(k[2]) / float(k[3])) * 100.0
            imiss = (float(k[4]) / float(k[5])) * 100.0
            if raw == True:
                out = format("%i,$i,$i,$i,$i,$i," % (k[0], k[1], k[2], k[3], k[4], k[5]))
            else:
                out += format("Branch mispred rate=%2.2f%%, L1I miss=%2.2f%%, L1D miss=%2.2f%%." % (brpred, imiss, dmiss))
            return out
    finally:
        sys.stdout.flush()

def do_get_perf (sock, mode, raw, arch):
    if arch == 'a':
        if mode == 'm': return do_arm64_toplev(sock, raw)
        if mode == 'p': return do_arm64_pipeline(sock, raw)
        return do_x86_64(sock, mode, raw)

def do_x86_64 (sock, mode, raw):
    query_toplev = "INST_RETIRED:TOTAL_CYCLES,INST_RETIRED:ANY_P,ICACHE:IFETCH_STALL,CYCLE_ACTIVITY:STALLS_LDM_PENDING\0"
    query_br = "BR_MISP_RETIRED,BR_INST_RETIRED,ICACHE:MISSES,ICACHE:HIT,MEM_LOAD_UOPS_RETIRED:L1_MISS,MEM_LOAD_UOPS_RETIRED:L1_HIT\0"
    try:
            k = sendQuery(sock, query_toplev)
            ipc = float(k[1]) / float(k[0])
            fes = float(k[2]) / float(k[0]) * 100.0
            bes = float(k[3]) / float(k[0]) * 100.0
            if raw == True:
                out = format("%2.2f,%2.2f,%2.2f," % (ipc, fes, bes))
            else:
                out = format("IPC=%2.2f - FRONTEND=%2.2f%% - BACKEND=%2.2f%%. " % (ipc, fes, bes))
            k = sendQuery(sock, query_br)
            brpred = (float(k[0]) / float(k[1])) * 100.0
            dmiss = (float(k[2]) / float(k[3])) * 100.0
            imiss = (float(k[4]) / float(k[5])) * 100.0
            if raw == True:
                out = format("%2.2f,%2.2f,%2.2f," % (brpred, imiss, dmiss))
            else:
                out = format("Branch mispred rate=%2.2f%%, L1I miss=%2.2f%%, L1D miss=%2.2f%%." % (brpred, imiss, dmiss))
            return out
    finally:
        sys.stdout.flush()

def main ():
    mode = 'm'
    coreid = -1
    sock = socket(AF_INET, SOCK_STREAM)
    server_address = (sys.argv[1], 9999)
    terminator = '\r'
    raw = False

    try:
        idx = 2
        while True:
            if(sys.argv[idx] == '-n'):
                terminator = '\n'
                idx+=1
                continue
            if(sys.argv[idx] == '-r'):
                raw = True
                idx+=1
                continue
            if(sys.argv[idx] == '-c'):
                idx+=1
                coreid = sys.argv[idx]
                idx+=1
                continue
            if(sys.argv[idx] == '-m'):
                idx+=1
                mode = sys.argv[idx]
                idx+=1

    except IndexError:
        idx=1

    if raw == False:
        curses.initscr()
        curses.noecho()
        curses.cbreak()
        stdscr = curses.initscr()

    sock.connect(server_address)

    arch = sock.recv(1)
    cmax = recv_bytes(sock, 4)
    if coreid != -1:
        write_int(sock, -10)
        write_int(sock, -9)
        write_int(sock, coreid)

    while True:
        out = do_get_perf (sock, mode, raw, arch)

        out += terminator
        if raw == True:
            sys.stdout.write(out)
            sys.stdout.flush()
        else:
            stdscr.clear()
            stdscr.addstr(out) #, curses.color_pair(1))
            stdscr.refresh()
            c = stdscr.getch()
            if c == 'p':
                mode = 'p'
            if c == 'm':
                mode = 'm'

if __name__ == "__main__":
    main()

