# Cotre CO01D DMR radio
This is likely the cheapest DMR radio out there for about $20-$45. Although being marketed like an ISM radio, it is none. You need a license to operate this radio and there are some hefty fines awaiting you, if you don't have one. Moreover, it is generally not a good idea to mess with HAM radio operators. We turned triangulating transmitters into a [sport](https://en.wikipedia.org/wiki/Amateur_radio_direction_finding)

The protocol appears to be a weird one: It is actually a stream of requests and responses that do not alternate. That is, the CPS bombards the radio with requests and the radio responses to it some time later. Consequently, it is harder to correlate a response to its request. Although such a protocol is common in networks (e.g., TCP) it is rather uncommon for USB devices as there is virtually no latency between request and response and thus, the bandwidth is not limited by the latency. Is this what happens when a network engineer writes embedded code?

## XOff/XOn flow control and escaping
It appears as if the radio uses an XOff/XOn *software* flow control. The bytes `11` (XOn) and `13` (XOff) signal the CPS to start and stop transmission of data. Consequently, these bytes must be replaced by an escape sequence using `5c` (ASCII value of `\`) as an escape byte in the normal data flow. The decoding can be applied by the formula
```
 5c XX = (0x5c xor XX xor a3)
```
Thus `5c xor a3 xor a3 = 5c`, `5c xor ee xor a3 = 11` and `5c xor ec xor a3 = 13`. These control bytes (XOn/XOff) are only be observed after write requests. Moreover, these bytes are only send within a single USB frame. I've also seen the CPS bombarding the radio with write requests and ignoring the flow control completely. Consequently, I believe that the radio does not actually use these bytes for flow control but rather as an ACK when writing data to the radio. See *write request* below.


## Packet format (Layer 0)
Requests and responses appear to share the same packet format. Please note, that due to the nature of the chosen transport, a packet may be split over two USB transfer packets. 

```
   0                               8                               16                              24
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  | Preamble, fixed to 0xad       | 16 bit payload length, big endian                             | Payload, variable size     ... 
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
   ...  However, only a few bytes are actually transferred here.                                  | Checksum                      |
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
```
### Checksum
The checksum is a simple XOR of all payload bytes.

### Example
The very first request send to the device is 
```
 ad 00 07 ff 04 03 00 00 00 01 f9
```
This would then decode into 
  * Preamble `ad` 
  * Length `0007`
  * Payload `ff 04 03 00 00 00 01`
  * CRC `f9` = `ff ^ 04 ^ 03 ^ 01`


## Requests and Responses (Layer 1)
The Layer 0 does not contain any commands or type-fields that allow for different operations like *read*, *write* , *enter program mode*, *reboot* etc. Consequently, these commands must be 
implemented in another layer within the payload of these packets. The majority of payload lengths is very short. Like 6 or 7 bytes. Consequently, there is only little room for a proper state-less protocol. I.e., commands like *read n bytes from address x*. Moreover, there are a large number of packets being exchanged before the actual codeplug read and write operations start. I therefore fear, that the underlying protocol for reading and writing the codeplug is stateful. This makes the reverse engineering of the protocol much harder as the meaning of specific bytes in a packet may change from state to state.  

### Read request (opcode 02)
These packets are not only used to read the codeplug from the device but also to read additional information from memory. For example, one of the very first read requests will read a memory address from the device. 

Each read request reads only 4 bytes of data. Given the 6 bytes overhead from the request and 4 bytes overhead from the packet, the efficiency is < 30%. That is, 2/3 of transferred data is overhead.

The protocol implements a stream-like behavior. The CPS will send several requests to the radio without waiting for a response from the radio for each request. To dispatch the stream of responses received back from the device, a sequence number is used.

#### Read request
```
    0                               8                               16                              24
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
0  | Fixed to 0xff                 | Opcode 0x02                   | Address, 32bit little-endian                               ...
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
4   ...                                                            | Sequence number               |
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
```
For read requests, the first two bytes are fixed to `0xff` and `0x02`, where the former is fixed for all requests and responses while the second appears to be the request type or opcode. The address to read from is given as a 32bit integer in little-endian. The last byte appears to be a sequence number, when reading a *larger* block of data. That is, more than 4 bytes at once. The sequence number appears to be in the range [1,128], I've never observed sequence number 0 or any number larger than 128. When the sequence number reaches 128, it wraps around to 1.

#### Read response
```
    0                               8                               16                              24
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
0  | Fixed to 0xff                 | Sequence number               | Data                                                       ...
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
4   ...                                                            |
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
```
The response consists of the fixed byte `0xff` (common for all packets) followed by the sequence number of the request. After the sequence number, the actual data follows. The response data length appears to be fixed to 4 bytes.


### Program mode request (opcode 04)
#### Request 
This request is send at the very beginning before reading and writing the codeplug. The request is send several times (observed 2) and responded only once by the radio.
```
 ff 04 03 00 00 00 01
```
As this request is the very first one send to the device, it is likely a command to put the radio into programming mode.

#### Response 
The response payload is simply
```
 ff 01 80
```


### Command requests (opcode 84)
These requests only appear in conjecture with write requests to some *special* memory or at the very beginning of the communication with the radio. There is no response from the radio to any of these command requests.
#### Request 
The command `ff 84 03 00 00 00 80` is send 20 times before any other command is send to the radio. As the radio uses its serial interface for debugging too, this might disable this feature, as it may interfere with the codeplug read or write. 

The command `ff 84 05 00 00 00 00` is send before any write request to the *special* memory location `8200ad04` and the command `ff 84 05 00 00 00 a5` is send immediately thereafter.

From this, we might interpret the command request structure as
```
    0                               8                               16                              24
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
0  | Fixed to 0xff                 | Opcode 0x84                   | Command                       | Set to 0                      |
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
4  | Set to 0                      | Set to 0                      | Argument                      |
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
```
The first two bytes are fixed to `ff` and `84` followed by the command byte. The next 3 bytes are fixed to 0 while the last byte may carry an argument to the command. Due to the limited use of these command requests, it is not clear whether this structure is valid or not. 

#### Response
This request is never responded by the device.


### Write request (opcode 83)
Write requests are used twofold. They are used to write the actual codeplug to the device but also to write some cryptic data into *special* memory locations.

#### Request 
```
    0                               8                               16                              24
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
0  | Fixed 0xff                    | Opcode 0x83                   | Address 32bit little endian                                ...
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+  
4   ...                                                            | Data, variable length                                      ...
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+  
    ...                                                                                                                            |
   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 ```
As usual, the first two bytes are fixed to `ff` and `83` followed by the address to write to. The remaining data of the request is the data to write. The length of the data is variable and request with up to 128 bytes has been observed. The packet format, however, allows for larger payloads.

#### Response
The radio does not respond to write requests directly with a packet but rather sends a XOff+XOn control byte sequence, that is `13 11`. As these two bytes are always observed within a single USB frame, I assume that the radio does not perform flow control with these chars but rather uses them to signal an write ACK.


## Request sequences
There are a lot of request sequences send before the actual codeplug read and write begins. Many of these requests appear to prepare the radio and obtain some memory addresses.

### Common init sequence
The first request send is the `04` one. That is, the CPS sends several (I've seen 2) `ff 04 03 00 00 00 01` requests to the radio until it responds with `ff 01 80`.

Then, the CPS bombards the radio with `ff 84 03 00 00 00 80` commands. That is, I've seen 20 requests send to the radio. Please note that the radio never responds to these requests. So I believe that this request is send 20 time all the time. The meaning of this request is still unknown, but likely prepares the radio for communication. 

Then the CPS send several (I've seen 3) read requests from address `81c00270`. It is likely, that the read request is send until a response from the device is received. The radio responds with a memory address. I've seen `8200ad04` and I guess that this address is fixed for this radio and at least firmware version. This address is like some sort of memory location to control the internal state of the radio. In what follows, there are a lot of write requests to this memory location.

Then, a sequence of command and write requests are send to the device. That is, 

  * the CPS sends the command `ff 84 05 00 00 00 00` to the radio. The radio does not respond to this command at all. 
  * The CPS sends a write request to `8200ad04` (the address we just obtained) with the content `aa 06 0a 06 0a bb 00 00` which is ACKed using the XOff XOn chars. 
  * Then the CPS sends another `84` command `ff 84 05 00 00 00 a5`. 

This sequence is send 20 times. Nothing changes in these 20 rounds. The same address, same content send and same response from the radio.

Then, two memory addresses are read. That is,
  * first `81c00268` is read and the radio returns `8201974c`. 
  * Then `81c0026c` is read and the radio returns `8200ac2c`.

Then, another write request to `8200ad04` with the content `aa 06 0a 08 04 bb 00 00` (same address and content as before) is send (including the preceding and trailing `84` commands). This write command is send only once.

Then, 8 bytes are read from address `8200ac2c` (the address we just obtained). The device returns `aa 0a 0a 08 00 00 00 00`. 

Then, another write request (including the `84` commands) from address `8200ad04` is send, but this time with some other content `aa 07 00 2b 00 2c bb 00 00 00`. This data is only written once.

Then, the previous 8 bytes are read again from `8200ac2c`. This time, however, the radio returns `aa 07 80 2b 02 ae bb 00`.

Here ends the common sequence between reading and writing the codeplug to and from the device. Although reading and writing continues with almost the same requests, there are subtile differences between reading and writing.

### Read codeplug 
Reading the codeplug continues with another write sequence (including the `84` commands) to the address `8200ad04`. But this time, each write command write some different data to the same address
  * First request writes `aa 06 0a 07 0b bb 00 00`, 
  * the second request writes `aa 06 0a 03 0f bb 00 00` and
  * the last request writes `aa 06 0a 00 0c bb 00 00`.
This sequence is almost identical to the sequence in a codeplug write. The content of the second write request, however, differs.

Then the CPS reads 12 bytes from `8201974c` (an address we received before). The radio delivers `84 65 00 82 04 03 00 00 54 6d 00 82`. The first and last 4 bytes might be addresses `82006584` and `82006d54`. These requests and responses are identical to the codeplug write.

Finally, the actual codeplug read starts from address `82006584`, an address we received before.

After reading the entire codeplug from the device, the CPS sends two write requests to the device (include the `84` commands).
  * The first writes `aa 06 0a 04 08 bb 00 00` to `8200ad04` and 
  * the second writes `aa 06 0a 07 0b bb 00 00` to the same address. 

### Write codeplug 
Writing the codeplug continues with another write sequence (including the `84` commands) to the address `8200ad04`. But this time, each write command write some different data to the same address
  * First request writes `aa 06 0a 07 0b bb 00 00`, 
  * the second request writes `aa 06 0a 01 0d bb 00 00` and
  * the last request writes `aa 06 0a 00 0c bb 00 00`.
This sequence is almost identical to the sequence in a codeplug read. The content of the second write request, however, differs.

Then the CPS reads 12 bytes from `8201974c` (an address we received before). The radio delivers `84 65 00 82 04 03 00 00 54 6d 00 82`. The first and last 4 bytes might be addresses `82006584` and `82006d54`. These requests and responses are identical to the codeplug read.

The CPS continues with reading 772bytes of the from address `82006584`. This matches the beginning of the codeplug read, however, here not the complete codeplug is read.

The actual codeplug write starts with a write request to address `82006d54` (another address we received before) writing 128 bytes at once. These write requests are not encapsulated in `84` commands.

After the codeplug has been written to the device, another write sequence to address `8200ad04` with the content `aa 06 0a 02 0e bb 00 00` is performed, includeing the `84` commands. This likely triggers a reboot.


## Some *special* addresses
There are some special addresses being read from and written to during the codeplug read and write. Some of these addresses are pointers, other likely contain status and control registers. Addresses read from the device are marked in *italic*. These addresses may change between devices or firmware revisions and should not be assumed static.

| Address    | RW | Size | Content                                                       |
| ---------- | -- | ---- | ------------------------------------------------------------- |
| 81c00268   | R  | 4    | Content table address. E.g., *8201974c*.                      |
| 81c0026c   | R  | 4    | Status register address. E.g., *8200ac2c*.                    |
| 81c00270   | R  | 4    | Control register address. E.g., *8200ad04*.                   |
| *82006584* | R  | 772  | Some additional radio info.                                   |
| *82006d54* | RW | ???  | Codeplug content address.                                     |
| *8200ac2c* | R  | 8    | Likely some sort of status register of the firmware.          |
| *8200ad04* | W  | 10   | Likely the *control register* for the firmware.               |
| *8201974c* | R  | 12   | Info addr *82006584* & length 772, codeplug addr *82006d54*.  |


## Scripts
The `extract.py` script will extract and partially interpret the requests and responses from the host/device captured using wireshark. The `extract.py` file contains the script, while the `packet.py` file implements the protocol and packet format.
