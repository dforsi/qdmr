#!/usr/bin/env python3

import pyshark
import struct
import sys
import binascii
from packet import Stream, Packet, hexDump

def isFromHost(p):
    return ("host" == p.usb.src) 

def isToHost(p):
    return ("host" == p.usb.dest) 

def isDataPacket(p):
  return ("USB.CAPDATA" in p)

def getData(p):
  if not isDataPacket(p): 
    return None
  return binascii.a2b_hex(p["USB.CAPDATA_RAW"].value)



if len(sys.argv) < 3:
  print("""USAGE: extract.py WHAT PCAPFILE
  
  WHAT: What to extract 
    - 'raw' dumps raw data streams.
    - 'packets' dumps packet structure.

  PCAPFILE: The pcap file (compressed or not). """)
  sys.exit(-1)


cap = pyshark.FileCapture(sys.argv[2], include_raw=True, use_json=True)
what = sys.argv[1]


if "raw" == what:
  for p in cap:
    if isDataPacket(p):
      if isFromHost(p):
        print("> " + hexDump(getData(p)))
      else: 
        print("< " + hexDump(getData(p)))


elif "packet" == what:
  in_str  = Stream()
  out_str = Stream()

  for p in cap:
    if isDataPacket(p):
      if isFromHost(p):
        out_str.appendData(getData(p))
        while out_str.hasPacket():
          pkt = out_str.popPacket();
          print(pkt.dump("> "))
      else:
        in_str.appendData(getData(p))
        while in_str.hasPacket():
          pkt = in_str.popPacket();
          print(pkt.dump("< "))
        
