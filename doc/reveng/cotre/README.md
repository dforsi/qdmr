# Cotre CO01D DMR radio
This is likely the cheapest DMR radio out there for about $20-$45. 

The protocol appears to be a weird one: It is actually a stream of requests and responses that do not alternate. That is, the CPS bombards the radio with requests and the radio responses to it some time later. Consequently, it is harder to correlate a response to its request. Although such a protocol is common in networks (e.g., TCP) it is rather uncommon for USB devices as there is virtually no latency between request and response and thus, the bandwidth is not limited by the latency. Is this what happens when a network engineer writes embedded code?

## Packet format (Layer 0)
Requests and responses appear to share the same packet format. Please note, that due to the nature of the chosen transport, a packet may be split over two USB transfer packets. 

```
   0                               8                               16                              24
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  | Preamble, fixed to 0xad       | 16 bit payload length, big endian                             | Payload, variable size     ... 
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
   ...  However, only a few bytes are actually transferred here.                                  | Checksum?                     |
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
```
### Checksum
The checksum appears to be a simple XOR of all payload bytes.

### Example
The very first request send to the device when reading is 
```
 ad 00 07 ff 04 03 00 00 00 01 f9
```
The would then decode into 
  * Preamble `ad` 
  * Length `0007`
  * Payload `ff 04 03 00 00 00 01`
  * CRC `f9` = `ff ^ 04 ^ 03 ^ 01`


### Weird escaping during transfer for bytes 0x11 and 0x13
After some guesswork, I was able to figure that one out. It appears as if the bytes 0x11 and 0x13 are being replaced by an escape sequence using 0x5c as an escape byte. The decoding can be applied by the formula
```
 5c XX = (0x5c xor XX xor a3)
```
Thus `5c xor a3 xor a3 = 5c`, `5c xor ee xor a3 = 11` and `5c xor ec xor a3 = 13`. 

#### Why `5c` as escape char? 
Because it is the ASCII value of `\`.

#### Why escaping `11` and `13`?
I have absolutely no clue. Unlucky numbers?!? They do not match the start sequence of the packets. Avoiding the start sequence byte is usually the reason to escape bytes in a stream. 


## Requests and Responses (Layer 1)
The Layer 0 does not contain any commands or type-fields that allow for different operations like *read*, *write* , *enter program mode*, *reboot* etc. Consequently these commands must be 
implemented as another layer within the payload of the packets. The majority of payload lengths are very short. Like 6 or 7 bytes, consequently there is only little room for a proper state-less protocol. E.g., commands like *read n bytes from address x*. Moreover, there are a large number of packets being exchanged before the actual codeplug read and write operations start. I therefore fear that the underlying protocol for reading and writing the codeplug is stateful. 

## Scripts
The `extract.py` script will extract and partially interpret the requests and responses from the host/device captured using wireshark. The `extract.py` file contains the script, while the `packet.py` file implements the protocol and packet format.
