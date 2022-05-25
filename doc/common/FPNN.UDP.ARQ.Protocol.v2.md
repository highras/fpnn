## FPNN UDP ARQ Protocol

### Fields

FPNN UDP Package Header Structure:

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+

**Assembled** type excepted.

#### VERSION:

Data type: unsigned byte (uint8_t).

Current: 0x2.

#### Type:

Data type: unsigned byte (uint8_t).

* DATA: 0x01

* ACK/ACKS: 0x02

* UNA: 0x03

* ECDH: 0x04

* HEARTBEAT: 0x05

* FORCESYNC: 0x06

* CLOSE: 0x0F

* COMBINED: 0x80

* ASSEMBLED: 0x81

ACK/ACKS, UNA, HEARTBEAT, FORCESYNC, CLOSE are discardable; ECDH is reliable.

COMBINED and DATA can be reliable or discardable.


#### FLAG:

Data type: unsigned byte (uint8_t).

* Discardable: 0x01

* Monitored: 0x02 (Discarded)

* Segmented: mask: 0x0C

	+ 0x00: no segmented
	+ 0x04: total & index in 1 Byte (Max 128 segments.)
	+ 0x08: total & index in 2 byte (Max 32767 segments.)
	+ 0x0C: total & index in 4 byte (Max 2147483647 segments.)

	If the 0x10 of **FLAG** is set means this segment is the last segment.

	Index for segment begin from 1.

	UDP sequence number for each segments are difference.

	Two bytes (big endian) will be added for segmented package for indicating the sliced package ID.

	If any sliced data length is Zero, means the original data are invalidated, which may cause by one of slices is expired.

* Last Segment Data: 0x10

* First DUP Package: 0x20

	First DUP Package flag cannot set with Discardable flag.

* Expired / Disabled / Cancelled Package: 0x80

	Package body can be zero or ignored, only sync for reliable UDP seq.

#### Sign:

The Tiny Simple Hash result for the package.

The sign value of first reliable or monitored package is random number.

Random for the unmonitored discardable packages which generated before the first reliable or monitored package.

Tiny Simple Hash:

The hash only using for verifying, and require it cannot be reverse inference, not care the collision rate.

* Calculation:

	Params:
	
	+ fseq: The UDP seq of first reliable or monitored package
	
	+ sign: the sign value for the first reliable or monitored package

	+ cseq: The UDP seq for current package

	Process:

		byte rsign = ~sign
		byte[4] fseqBE = fseq
		byte[4] cseqBE = cseq

		fseqBE[1] ^= sign
		fseqBE[3] ^= sign

		cseqBE[0] ^= rsign
		cseqBE[2] ^= rsign

		byte cyc1 = sign % 32
		byte cyc2 = rsign % 32

		uint32 f = fseqBE
		uint32 c = cseqBE

		uint32 newF = (f >> cyc1) | (f << (32 - cyc1))
		uint32 newC = (c << cyc2) | (c >> (32 - cyc2))

		byte[4] res = newF ^ newC

		tiny simple hash value = res[0] + res[1] + res[2] + res[3]


#### UDP Seq:

Data type: unsigned int (uint32_t), big endian.

Desc:
	
First/init value is random.  
Reliable packages & monitored discardable packages is in continuous sequence numbers.  
Unmonitored discardable packages using random numbers.  

Any package will be resent has to use a same sequence number.  


### Packages

#### Type: ECDH

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	| Params |                          |
	+--------+                          |
	|                                   |
	|        ... Public Key ...         |
	|                                   |
	+--------+--------+--------+--------+

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	| Params |                          |
	+--------+                          |
	|                                   |
	|        ... Public Key ...         |
	|                                   |
	+--------+--------+--------+--------+
	| Params |                          |
	+--------+                          |
	|                                   |
	|       ... Public Key 2 ...        |
	|                                   |
	+--------+--------+--------+--------+

* Params:

		  Byte 1  
		 01234567 
		+--------+
		| Params |
		+--------+

	+ bit 0: 0: 128 bits key; 1: 256 bits keys
	+ bit 1 ~ bit 7: Public Key length

* Public Key:

	Using for the package encryption

* Public Key 2:

	Using fot the enchance encryption

* Enhanced Encryption Mode:

	The enhanced encryption is the twice encrypting.
	The first encryption is the basic package encryption, the second encryption is only for reliable
	or monitored packages, and using the stream encryption mode, and only encrypting the data contents.
	The unmonitored discardable packages only apply the basic package encryption, don't apply the second encrypting.

* Generate IV:

		IV = md5(secret)

	IV is in binary foramt, not the hex format.

* Generate key:

	+ 128 bits key:

		The first 16 bytes of secret.

	+ 256 bits key:

		- If the secret is 32 bytes, the secret is the key;
		- It the secret is len then 32 bytes, `key = sha256(secret)`.

#### Type: DATA

##### Unsegmented

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|                                   |
	|           ... DATA ...            |
	|                                   |
	+--------+--------+--------+--------+

##### Segment FLAG: 0x04

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|   Package ID    | segIdx |        |
	+--------+--------+--------+        +
	|                                   |
	|           ... DATA ...            |
	|                                   |
	+--------+--------+--------+--------+

##### Segment FLAG: 0x08

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|   Package ID    |  segment Index  |
	+--------+--------+--------+--------+
	|                                   |
	|           ... DATA ...            |
	|                                   |
	+--------+--------+--------+--------+

##### Segment FLAG: 0x0C

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|   Package ID    |  segment Index  |
	+--------+--------+--------+--------+
	|  segment Index  |                 |
	+--------+--------+                 |
	|                                   |
	|           ... DATA ...            |
	|                                   |
	+--------+--------+--------+--------+

#### Type: ACK/ACKS

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|          ACK UDP Seq 1            |
	+--------+--------+--------+--------+
	|          ACK UDP Seq 2            |
	+--------+--------+--------+--------+
	|          .............            |
	+--------+--------+--------+--------+
	|          ACK UDP Seq N            |
	+--------+--------+--------+--------+

#### Type: UNA

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|           UNA UDP Seq             |
	+--------+--------+--------+--------+


#### Type: ASSEMBLED

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+
	|Version |  Type  |
	+--------+--------+
	+--------+--------+
	| package 1 length|
	+--------+--------+
	         +--------+--------+--------+
	         | Type 1 | FLAG 1 | Sign 1 |
	+--------+--------+--------+--------+
	|    Assembled Package 1 UDP Seq    |
	+--------+--------+--------+--------+
	|     Assembled Package 1 body      |
	+--------+--------+--------+--------+
	+--------+--------+
	| package N length|
	+--------+--------+
	         +--------+--------+--------+
	         | Type N | FLAG N | Sign N |
	+--------+--------+--------+--------+
	|    Assembled Package N UDP Seq    |
	+--------+--------+--------+--------+
	|     Assembled Package N body      |
	+--------+--------+--------+--------+

Assembled type without Flag field, Sign field and UDP seq field.

* **Assembled Package Header**

		  Byte 1   Byte 2   Byte 3   Byte 4
		 01234567 01234567 01234567 01234567
		+--------+--------+
		|Version |  Type  |
		+--------+--------+

* **Assembled Section**

		  Byte 1   Byte 2   Byte 3   Byte 4
		 01234567 01234567 01234567 01234567
		+--------+--------+
		| package 1 length|
		+--------+--------+
		         +--------+--------+--------+
		         | Type 1 | FLAG 1 | Sign 1 |
		+--------+--------+--------+--------+
		|    Assembled Package 1 UDP Seq    |
		+--------+--------+--------+--------+
		|     Assembled Package 1 body      |
		+--------+--------+--------+--------+

	After the first two fields of the assembled package header, there are the assembled package sections.

	Each assembled section has 16 bits unsigned integer (uint16_t) indicated the section length in big endian.  
	Which length excludes the package length field, includes the assembled Type, Flag, Sign, UDP Seq, and assembled package body.

	**Notice**

	+ The firdst reliable package with the **First UDP Package** flag (0x20) cannot be assembled.

#### Type: COMBINED

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	+--------+--------+--------+--------+
	|  Type  |  FLAG  | Component Bytes |
	+--------+--------+--------+--------+
	+--------+--------+--------+--------+
	| ECDH or ACK/ACKS or UNA Component |
	+--------+--------+--------+--------+
	+--------+--------+--------+--------+
	|  Type  |  FLAG  | Component Bytes |
	+--------+--------+--------+--------+
	+--------+--------+--------+--------+
	|          DATA component 1         |
	+--------+--------+--------+--------+
	+--------+--------+--------+--------+
	|  Type  |  FLAG  | Component Bytes |
	+--------+--------+--------+--------+
	+--------+--------+--------+--------+
	|          DATA component N         |
	+--------+--------+--------+--------+

**Only** six types combined packages.

* ECDH with DATA (with DATA ...)
* ACK/ACKS with DATA (with DATA ...) (with CLOSE)
* UNA with DATA (with DATA ...) (with CLOSE)
* DATA with DATA ... (with CLOSE)
* UNA with ACK/ACKS (with DATA ...) (with CLOSE)
* FORCESYNC (with UNA) (with ACK/ACKS) (with DATA ...)

Other combinations are **no allowed**.

For combined package, the first component is ECDH or ACK/ACKS or UNA or FORCESYNC or DATA component.  
And all followed components **ONLY** can be DATA components.

**If the first component is ECDH, all followed components are encrypted.**

* Component Bytes:

	Big endian.
	
	Exclude the 4 bytes for Type, FLAG, Component Bytes.

#### Type: HEARTBEAT

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|    Timestamp in ms for sending    |
	|               64 bits             |
	+--------+--------+--------+--------+

Timestamp in big endian.

#### Type: FORCESYNC

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+

#### Type: CLOSE

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |  Sign  |
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+

If close in immediately, `FLAG` maybe set as discardable; if close in order, `FLAG` must be set as reliable.
