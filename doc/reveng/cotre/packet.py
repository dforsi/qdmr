from struct import unpack

def hexDump(s):
  h = " ".join(map("{:02x}".format, s))
  t = ""
  for i in range(len(s)):
    c = s[i]
    if c>=0x20 and c<0x7f:
      t += chr(c)
    else:
      t += "."
  return( h + " | " + t)


class Packet:
  TYPE_ACK = 0
  TYPE_PACKET = 1

  def __init__(self, type, payload=None, chk=None):
    self._type = type 
    self._payload = payload
    self._checksum = chk 
    if (None == self._checksum) and (Packet.TYPE_ACK != self._type):
      self._checksum = self.getChecksum()

  def __len__(self):
    if Packet.TYPE_ACK == self._type:
      return 2
    return 4 + len(self._payload)

  def getChecksum(self):
    if Packet.TYPE_ACK == self._type:
      return None 
    if None == self._payload:
      return None 
    chk = 0
    for i in len(self._payload):
      chk ^= self._payload[i]
    return chk 

  def dump(self, prefix=""):
    if Packet.TYPE_ACK == self._type:
      return prefix + "ACK"
    return "{0}PKT: len={1}, chk={2:x}\n{3}".format(
      prefix, len(self._payload), self._checksum, self._payload.dump(prefix + " | "))

  @staticmethod 
  def unpack(data):
    if (len(data)>=2) and (0x13 == data[0]) and (0x11 == data[1]):
      return Packet(Packet.TYPE_ACK)
    if (len(data)>=4) and (0xad == data[0]):
      preamble, length = unpack(">BH", data[:3])
      return Packet(Packet.TYPE_PACKET, payload=Payload(data[3:(3+length)]), chk=data[3+length])
    print("Oops: packet {0:X}".format(data[0]))


class Payload:
  def __init__(self, data):
    self._data = data

  def __len__(self):
    return len(self._data)

  def dump(self, prefix=""):
    return "{0}Payload: len={1}\n{0} | {2}".format(prefix, len(self._data), hexDump(self._data))


class Stream:
  def __init__(self):
    self._buffer = bytearray()

  def appendData(self, data):
    self._buffer += bytearray(data)
  
  def hasPacket(self):
    if len(self._buffer)<2:
      return False
    # Handle ACK
    if (0x13 == self._buffer[0]) and (0x11 == self._buffer[1]):
      return True
    # handle 'normal' packets
    if len(self._buffer)<4:
      return False
    preamble, length = unpack(">BH", self._buffer[:3])
    return len(self._buffer) >= (4+length)

  def popPacket(self):
    if not self.hasPacket():
      return None 
    packet = Packet.unpack(self._buffer)  
    self._buffer = self._buffer[len(packet):]
    return packet



