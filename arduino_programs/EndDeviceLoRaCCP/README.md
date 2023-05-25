
# Schedule
The events will be executed as part of a cascade effect beginning with the transmission of a packet. Transmission procedures should be executed for every x time, and the cascade of events should be as follows:

1. Begin packet with implicit header
2. Create ciphertext
	1. Generate msg_key
	2. Increment counter according to counter scheme
	3. Return (payload XOR msg_key)
3. Create MIC
4. Add device address, sequence number, ciphertext and MIC to packet
5. Increment sequence number
6. Finish and send packet using non-blocking mode
	1. ??????? What does non-blocking mode do exactly? 
7. When the transmission is done onTxDone() will be envoked
	1. Standby in x time
	2. Call rxMode() function
8. rxMode() configure the chip for receivals
	1. Enable inverted IQ since gateways usually send with inverted IQ signal
	2. ?????????
9. Turn on idle-mode
10. Disable inverted IQ



90. If something is received => Generate new secret key










# Throw out


2. Call LoRa.receive(int size) with the payload size specified to enable implicit mode
		1. Changes the mapping of DIO0 to ???????????
		2. Sets the payload length register (RegPayloadLength) to the specified size
		3. Changes the operation mode register (RegOpMode) to continuous receive mode where the modem scans the channel continuously for a preamble.
		4. When the preamble is detected it tracks it until the packet is fully received and when it reception of a valid header CRC the RxDone interrupt is set. 