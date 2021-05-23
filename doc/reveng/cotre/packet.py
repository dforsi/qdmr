from __future__ import annotations
import struct


def hexDump(s: bytes, prefix="", addr=0) -> str:
  """ Utility function to hex-dump binary data. """
  N = len(s)
  Nb = N//16
  if (N%16): Nb += 1
  res = ""
  for j in range(Nb):
    a,b = j*16, min((j+1)*16,N)

    h = " ".join(map("{:02x}".format, s[a:b]))
    h += "   "*(16-(b-a))
    t = ""
    for i in range(a,b):
      c = s[i]
      if c>=0x20 and c<0x7f:
        t += chr(c)
      else:
        t += "."

    res += (prefix + "{:08X} ".format(addr+16*j) + h + " | " + t + "\n")
  return res[:-1]


class Packet:
  TYPE_ACK = 0
  TYPE_PACKET = 1
  TYPE_UNKNOWN = 2

  def __init__(self, type, payload=None, chk=None):
    self._type = type 
    self._payload = payload
    self._checksum = chk 
    if (None == self._checksum) and (Packet.TYPE_PACKET == self._type):
      self._checksum = self.getChecksum()

  def __len__(self) -> int:
    if Packet.TYPE_ACK == self._type:
      return 2
    elif Packet.TYPE_UNKNOWN == self._type:
      return len(self._payload)
    return 4 + len(self._payload)

  def getChecksum(self) -> int:
    if (Packet.TYPE_ACK == self._type) or (Packet.TYPE_UNKNOWN == self._type):
      return None 
    if None == self._payload:
      return None 
    chk = 0
    data = self._payload.pack()
    for i in range(len(self._payload)):
      chk ^= data[i]
    return chk 

  def dump(self, prefix="") -> str:
    # Dump ACK packets
    if Packet.TYPE_ACK == self._type:
      return prefix + "ACK\n"
    # Dump unknown data
    if Packet.TYPE_UNKNOWN == self._type:
      return "{0}UKN: len={1}\n{2}\n".format(
        prefix, len(self._payload), self._payload.dump(prefix+" "))
    # Dump proper packets
    crc_fmt = "{0:02x}".format(self._checksum)
    crc_comp = self.getChecksum()
    if self._checksum == crc_comp:
      crc_fmt += " (OK)"
    else:
      crc_fmt += " (\x1b[1;31mERR: {:02X}\x1b[0m)".format(crc_comp)
    return "{0}PKT: len={1}, chk={2}\n{3}\n".format(
      prefix, len(self._payload), crc_fmt, self._payload.dump(prefix + " "))

  def pack(self) -> bytes:
    if Packet.TYPE_ACK == self._type:
      return b"\x13\x11"
    elif Packet.TYPE_UNKNOWN == self._type:
      return self._payload.pack()
    return struct.pack(">BH", 0xad, len(self._payload)) + self._payload.pack() + struct.pack("B", self.getChecksum())

  @staticmethod 
  def unpack(data):
    # Handle unknown data
    if (0x13 != data[0]) and (0xad != data[0]):
      payload = bytearray()
      for i in range(len(data)):
        if (0x13 != data[i]) and (0xad != data[i]):
          payload.append(data[i])
        else:
          break
      return Packet(Packet.TYPE_UNKNOWN, payload=Payload(payload))
    # Handle ACKs
    if (len(data)>=2) and (0x13 == data[0]) and (0x11 == data[1]):
      return Packet(Packet.TYPE_ACK, payload=None)
    # Handle 'normal' packets
    if (len(data)>=4) and (0xad == data[0]):
      preamble, length = struct.unpack(">BH", data[:3])
      return Packet(Packet.TYPE_PACKET, payload=Payload(data[3:(3+length)]), chk=data[3+length])


class Payload:
  def __init__(self, data : bytes):
    self._data = data

  def __len__(self) -> int:
    return len(self._data)

  def dump(self, prefix="") -> str:
    return "{0}Payload: len={1}\n{0} | {2}".format(prefix, len(self._data), hexDump(self._data))

  def pack(self) -> bytes:
    return self._data


class Stream:
  def __init__(self):
    self._buffer = bytearray()

  def appendData(self, data:bytes):
    self._buffer.extend(bytearray(data))
  
  def hasPacket(self) -> bool:
    # handle garbage
    if len(self._buffer)>0 and (0x13!=self._buffer[0]) and (0xad!=self._buffer[0]):
      return True
    if len(self._buffer)<2:
      return False
    # Handle ACK
    if (0x13 == self._buffer[0]) and (0x11 == self._buffer[1]):
      return True
    # handle 'normal' packets
    if len(self._buffer)<4:
      return False
    preamble, length = struct.unpack(">BH", self._buffer[:3])
    return (0xad == self._buffer[0]) and len(self._buffer) >= (4+length)

  def popPacket(self):
    if not self.hasPacket():
      return None 
    packet = Packet.unpack(self._buffer)  
    self._buffer = self._buffer[len(packet):]
    return packet



