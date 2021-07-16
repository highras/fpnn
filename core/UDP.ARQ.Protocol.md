## PackageDelayStatistatic Protocol

### Fields

#### VERSION:

Data type: unsigned byte (uint8_t).

Current: 0x1.

#### Type:

Data type: unsigned byte (uint8_t).

* COMBINED: 0x80

* DATA: 0x01

* ACK/ACKS: 0x02

* UNA: 0x03

* ECDH: 0x04

* HEARTBEAT: 0x05

* FORCESYNC: 0x06

* CLOSE: 0x0F

ACK/ACKS, UNA, HEARTBEAT are discardable; ECDH is reliable.

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

* Last Segment Data: 0x10

* First DUP Package: 0x20

	First DUP Package flag cannot set with Discardable flag.

#### Factor/Fake Factor：
	
Random number for encrypted packages.

The Tiny Simple Hash result for the reliable or monitored package.

Random for unmonitored discardable package.

Tiny Simple Hash:

The hash only using for verifying, and require it cannot be reverse inference, not care the collision rate.

* Calculation:

	Params:
	
	+ fseq: The UDP seq of first reliable or monitored package
	
	+ factor: the reserved value for the first reliable or monitored package

	+ cseq: The UDP seq for current package

	Process:

		byte rfactor = ~factor
		byte[4] fseqBE = fseq
		byte[4] cseqBE = cseq

		fseqBE[1] ^= factor
		fseqBE[3] ^= factor

		cseqBE[0] ^= rfactor
		cseqBE[2] ^= rfactor

		byte cyc1 = factor % 32
		byte cyc2 = rfactor % 32

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
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	|       Public Key (32 byte)        |
	+--------+--------+--------+--------+

#### Type: DATA

##### Unsegmented

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |Reserved|
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
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|   Package Seq   | segIdx |        |
	+--------+--------+--------+        +
	|                                   |
	|           ... DATA ...            |
	|                                   |
	+--------+--------+--------+--------+

##### Segment FLAG: 0x08

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|   Package Seq   |  segment Index  |
	+--------+--------+--------+--------+
	|                                   |
	|           ... DATA ...            |
	|                                   |
	+--------+--------+--------+--------+

##### Segment FLAG: 0x0C

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|   Package Seq   |  segment Index  |
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
	|Version |  Type  |  FLAG  |Reserved|
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
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+
	|           UNA UDP Seq             |
	+--------+--------+--------+--------+

#### Type: COMBINED

**Only** six types combined packages.

* ECDH with DATA (with DATA ...)
* ACK/ACKS with DATA (with DATA ...) (with CLOSE)
* UNA with DATA (with DATA ...) (with CLOSE)
* DATA with DATA ... (with CLOSE)
* UNA with ACK/ACKS (with DATA ...) (with CLOSE)
* FORCESYNC (with UNA) (with ACK/ACKS) (with DATA ...)

Other combinations are **no allowed**.

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |Reserved|
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


For combined package, the first component is ECDH or ACK/ACKS or UNA or FORCESYNC or DATA component.  
And all followed components **ONLY** can be DATA components.

* Component Bytes:
	
	Exclude the 4 bytes for Type, FLAG, Component Bytes.


#### Type: HEARTBEAT

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |Reserved|
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
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+

#### Type: CLOSE

	  Byte 1   Byte 2   Byte 3   Byte 4
	 01234567 01234567 01234567 01234567
	+--------+--------+--------+--------+
	|Version |  Type  |  FLAG  |Reserved|
	+--------+--------+--------+--------+
	|           UDP Seq                 |
	+--------+--------+--------+--------+

If close in immediately, `FLAG` maybe set as discardable; if close in order, `FLAG` must be set as reliable.
