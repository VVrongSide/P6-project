#include <SPI.h>
#include <LoRa.h>

const long frequency = 915E6;

uint16_t sequenceNumber = 0;
uint16_t deviceAddress = 0x2B8;

void setup() {
	Serial.begin(115200);

	Serial.println("LoRaCCP v1");

	if (!LoRa.begin(frequency)) {
		Serial.println("Failed initializing LoRa connection...");
		while (1);
	}

	// set coding rate
	// set preamble length
	// enable crc

	// setup callback for onReceive - the callback will be triggered by an interrupt on _dio0 which calls onDio0Rise -> handleDio0Rise -> _onReceive
	// setup callback for onTxDone such that it can be put in receive mode af transmitting

	LoRa.idle();                          // set standby mode
	LoRa.disableInvertIQ();               // normal mode
}

void loop() {
	// wait for 2000 msec
	if (waited(2000)) {
		transmitMessage();
	}
		// put in tx mode
		// beginPacket(implicitHeader = 1)
		// piece together the packet content
		// endPacket(true)	// true = non-blocking mode
		// increment sequence number (maybe this needs to be before endPacket() if that envokes the onTxDone())

		// use onTxDone to enable IQ inversion and receive mode - the receive function should have the packet length as an argument to enable implicit mode
		// 
}

void transmitMessage() {
	byte header_b1 = deviceAddress >> 4;
	byte header_b2 = (deviceAddress << 4) or (sequenceNumber >> 8);
	byte header_b3 = sequenceNumber;
	uint16_t payload = getPayload();
	uint16_t ciphertext = getCiphertext(payload);
	uint32_t mic = getMIC(ciphertext);

	LoRa.beginPacket(1);
	LoRa.write(header_b1);
	LoRa.write(header_b2);
	LoRa.write(header_b3);
	LoRa.print(ciphertext);
	LoRa.print(mic);
	LoRa.endPacket(true);

	sequenceNumber++;
}

uint16_t getPayload() {
	return (uint16_t)random(65535);
}

uint16_t getCiphertext(uint16_t payload) {

	uint32_t msgKey = getMsgKey();
	uint16_t msgKey16 = msgKey;

	uint16_t ciphertext = payload ^ msgKey16;

	return ciphertext;
}

uint32_t getMsgKey() {

	// insert speck encryption

	return 23923;
}

uint32_t getMIC(uint16_t ciphertext) {

	// insert blake hashing

	return 1249816708;
}


bool waited(int interval) {
	static int currentTime = millis();
	int prevTime = 0;
	if (prevTime + interval >= currentTime) {
		prevTime = currentTime;
		return true;
	}
	return false;
}



// InvertIQ allows differentiating sender and receiver message.
// Usually, Gateways read messages with InvertIQ off and send messages with InvertIQ on, Nodes
// read messages with InvertIQ on and send messages with InvertIQ off.
// This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, 
// and Node never reads messages from other Node.