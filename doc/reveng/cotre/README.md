# Cotre DMR radio
This is likely the cheapest DMR radio out there for about $45. 

The protocol appears to be a weird one: It is actually a stream of requests and responses that do not alternate. That is, the CPS bombards the radio with requests and the radio responses to it some time later. Consequently, it is harder to correlate a response to its request. Although such a protocol is common in networks (e.g., TCP) it is rather uncommon for USB devices as there is virtually no latency between request and response and thus, the bandwidth is not limited by the latency. Is this what happens when a network engineer writes embedded code?

## Packet format
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

### Buggy CPS?!?
Sometimes, a request is send by the CPS that does not follow the format above. For example during read, I observed the following two consecutive requests
```
ad 00 07 ff 02 58 71 00 82 02 54
ad 00 07 ff 02 5c a3 71 00 82 03 51
```
The second appears to be one byte too long (both should have 7b payload). Also the checksum does not match. If, however, we ignore the `a3` byte in the middle, we obtain
```
ad 00 07 ff 02 58 71 00 82 02 54
ad 00 07 ff 02 5c 71 00 82 03 51
```
This appears to be a more reasonable request following the first one (i.e., identical request except for the last payload byte). More over, the checksums now match. So, it appears that the CPS is buggy assembling invalid requests. This reverse engineering will be fun!

## Extraction scripts
The `extract.py` script will extract and partially interpret the requests and responses from the host/device captured using wireshark. The `extract.py` file contains the script, while the `packet.py` file implements the protocol and packet format.
