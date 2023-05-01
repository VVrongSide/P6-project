#include <SPI.h>
#include <LoRa.h>

#define ROTL16(word, offset) (((word)<<(offset)) | (word>>(16-(offset))))
#define ROTR16(word, offset) (((word)>>(offset)) | ((word)<<(16-(offset))))
#define ROTL32(word, offset) (((word)<<(offset)) | (word>>(32-(offset))))
#define ROTR32(word, offset) (((word)>>(offset)) | ((word)<<(32-(offset))))
#define ROTL64(word, offset) (((word)<<(offset)) | (word>>(64-(offset))))
#define ROTR64(word, offset) (((word)>>(offset)) | ((word)<<(64-(offset))))

#define EncryptRound16(word1,word2,roundkey) (word1=(ROTR16(word1,8)+word2)^(roundkey), word2=ROTL16(word2,3)^word1)
#define EncryptRound32(word1,word2,roundkey) (word1=(ROTR32(word1,8)+word2)^(roundkey), word2=ROTL32(word2,3)^word1)
#define EncryptRound64(word1,word2,roundkey) (word1=(ROTR64(word1,8)+word2)^(roundkey), word2=ROTL64(word2,3)^word1)

#define DecryptRound16(word1,word2,roundkey) (word2=ROTR16(word2^=word1,3), word1=ROTL16((word1^=roundkey)-word2,8))
#define DecryptRound32(word1,word2,roundkey) (word2=ROTR32(word2^=word1,3), word1=ROTL32((word1^=roundkey)-word2,8))
#define DecryptRound64(word1,word2,roundkey) (word2=ROTR64(word2^=word1,3), word1=ROTL64((word1^=roundkey)-word2,8))

const long frequency = 915E6;

uint8_t rootKey[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
uint8_t secretKey[8];

uint16_t sequenceNumber = 0;
uint16_t deviceAddress = 0x2B8;
uint16_t counter = 0;					// for SPECK-CTR

bool test = true;

void setup() {
	Serial.begin(115200);

	Serial.println("LoRaCCP v1");

  if (!test) {
    if (!LoRa.begin(frequency)) {
      Serial.println("Failed initializing LoRa connection...");
      while (1);
    }
  }

	// set coding rate
	// set preamble length
	// enable crc

	// setup callback for onReceive - the callback will be triggered by an interrupt on _dio0 which calls onDio0Rise -> handleDio0Rise -> _onReceive
	// setup callback for onTxDone such that it can be put in receive mode af transmitting

  uint8_t testnonce[8] = {0x21, 0x43, 0x1a, 0x2c, 0x22, 0xca, 0xb5, 0x4d};
  deriveSecretKey(testnonce);

  if (!test) {
    LoRa.idle();                          // set standby mode
    LoRa.disableInvertIQ();               // normal mode 
  }
}

bool waited(int interval) {
  Serial.println("oh boi");
  static int currentTime = millis();
  int prevTime = 0;
  if (prevTime + interval >= currentTime) {
    prevTime = currentTime;
    return true;
  }
  return false;
}

void loop() {
	// wait for 2000 msec
	if (!test) {
    if (waited(2000)) {
      transmitMessage();
    }
	}
  delay(1000);
  transmitMessage();

}

void transmitMessage() {
	byte header_b1 = deviceAddress >> 4;
	byte header_b2 = (deviceAddress << 4) or (sequenceNumber >> 8);
	byte header_b3 = sequenceNumber;
	uint16_t payload = getPayload();
	uint16_t ciphertext = getCiphertext(payload);
	uint32_t mic = getMIC(ciphertext);

	LoRa.beginPacket(1);							// beginPacket(implicitHeader = 1)
	LoRa.write(header_b1);
	LoRa.write(header_b2);
	LoRa.write(header_b3);
	LoRa.print(ciphertext);
	LoRa.print(mic);
	LoRa.endPacket(true);							// endPacket(true)	// true = non-blocking mode

	sequenceNumber++;
}

uint16_t getPayload() {
	return (uint16_t)random(65535);
}

uint16_t getCiphertext(uint16_t payload) {

	uint32_t msgKey = getMsgKey();
	uint16_t msgKey16 = (uint16_t)msgKey;

  uint16_t ciphertext = payload ^ msgKey16;

  Serial.print("CIPHER: ");
  Serial.println(ciphertext);

	return ciphertext;
}

uint32_t getMIC(uint16_t ciphertext) {
  // insert blake hashing

  return (uint32_t)1249816708;
}

uint32_t getMsgKey() {

	uint8_t plaintext[4];
	uint32_t plaintextword[0];
	//uint8_t secretkey[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};	// replace with key retrieval
	uint32_t secretkeyword[0];
	uint32_t roundkey[22];
	uint8_t ciphertext[4];
	uint32_t ciphertextword[0];

	getAddressCounter(plaintext);                             // DeviceAddress + Counter (uint8 array of length 4)
	ConvertBW32(plaintext,plaintextword, 4);                  // input: uint8 array of length 4 // output: uint32 array of length 1
	ConvertBW32(secretKey,secretkeyword, 8);                  // input: uint8 array of length 8 // output: uint32 array of length 2
	Expand64(secretkeyword, roundkey);                        // input: uint32 array of length 2 // output: uint32 array of length 22
	Block32Encrypt(plaintextword, ciphertextword, roundkey);  // input: uint32 arrays // output: uint32 array
	ConvertW32B(ciphertextword, ciphertext, 4);

	uint32_t msgKey = ((uint32_t)ciphertext[0] << 24) | ((uint32_t)ciphertext[1] << 16) | 
						        ((uint32_t)ciphertext[2] << 8) | (uint32_t)ciphertext[3];

	return msgKey;
}

uint64_t deriveSecretKey(uint8_t nonce[]) {
  uint32_t nonceword[2];
  uint32_t rootKeyWord[4];
  uint32_t roundkey[27];
  uint8_t ciphertext[8];
  uint32_t ciphertextword[4];
  
  ConvertBW32(nonce,nonceword,8);
  ConvertBW32(rootKey,rootKeyWord, 16);
  Expand128(rootKeyWord, roundkey);
  Block64Encrypt(nonceword, ciphertextword, roundkey);
  ConvertW32B(ciphertextword, ciphertext, 8);

  uint32_t sKey = ((uint32_t)ciphertext[0] << 56) | ((uint32_t)ciphertext[1] << 48) | 
                  ((uint32_t)ciphertext[2] << 40) | ((uint32_t)ciphertext[3] << 32) |
                  ((uint32_t)ciphertext[4] << 24) | ((uint32_t)ciphertext[5] << 16) | 
                  ((uint32_t)ciphertext[6] << 8) | (uint32_t)ciphertext[7];
  Serial.println(sKey);
  return sKey;
}

// returns device address and counter - used as plaintext in SPECK
void getAddressCounter(uint8_t plaintext[]) {
  uint32_t plaintext32 = (uint32_t)(deviceAddress << 20) | (uint32_t)counter;
  for (int i = 0; i < 4; i++) {
    plaintext[i] = (uint8_t)(plaintext32 >> 8*i);
  }
  counter++;
}

void ConvertBW32(uint8_t bytes[], uint32_t words[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount/4; i++) {
    words[i] =  (uint32_t)bytes[j] | ((uint32_t)bytes[j+1]<<8) | 
          ((uint32_t)bytes[j+2]<<16) | ((uint32_t)bytes[j+3]<<24);
    j += 4;
  }
}

void Expand64(uint32_t K[], uint32_t roundkey[]) {
  uint32_t i, D=K[3], C=K[2], B=K[1], A=K[0];
  for (i = 0; i < 22;) {
    roundkey[i] = A;
    EncryptRound32(B,A,i++);
    roundkey[i] = A;
    EncryptRound32(C,A,i++);
    roundkey[i] = A;
    EncryptRound32(D,A,i++);
  }
}

void Block32Encrypt(uint32_t plaintext[], uint32_t ciphertext[], uint32_t roundkey[]) {
  uint16_t i;

  ciphertext[0] = plaintext[0];
  ciphertext[1] = plaintext[1];
  for (i = 0; i < 22;) {
    EncryptRound16(ciphertext[1], ciphertext[0], roundkey[i++]);
  }
}

void ConvertW32B(uint32_t words[], uint8_t bytes[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount; i++) {
    bytes[j] =   (uint8_t)words[i];
    bytes[j+1] = (uint8_t)(words[i] >> 8);
    bytes[j+2] = (uint8_t)(words[i] >> 16);
    bytes[j+3] = (uint8_t)(words[i] >> 24);
    j += 4;
  }
}


/*************** SPECK ***************/

void ConvertW64B(uint64_t words[], uint8_t bytes[], int wordcount) {
	int i, j = 0;
	for (i = 0; i < wordcount; i++) {
	  bytes[j] =   (uint8_t)words[i];
		bytes[j+1] = (uint8_t)(words[i] >> 8);
		bytes[j+2] = (uint8_t)(words[i] >> 16);
		bytes[j+3] = (uint8_t)(words[i] >> 24);
		bytes[j+4] = (uint8_t)(words[i] >> 32);
		bytes[j+5] = (uint8_t)(words[i] >> 40);
		bytes[j+6] = (uint8_t)(words[i] >> 48);
		bytes[j+7] = (uint8_t)(words[i] >> 56);
		j += 8;
	}
}

void ConvertBW64(uint8_t bytes[], uint64_t words[], int wordcount) {
	int i, j = 0;
	for (i = 0; i < wordcount/8; i++) {
		words[i] = 	(uint64_t)bytes[j] | ((uint64_t)bytes[j+1]<<8) | 
					((uint64_t)bytes[j+2]<<16) | ((uint64_t)bytes[j+3]<<24) | 
					((uint64_t)bytes[j+4]<<32) | ((uint64_t)bytes[j+5]<<40) | 
					((uint64_t)bytes[j+6]<<48) | ((uint64_t)bytes[j+7]<<56);
		j += 8;
	}
}

// Key schedules

void Expand128(uint32_t K[], uint32_t roundkey[]) {
	uint32_t i, D=K[3], C=K[2], B=K[1], A=K[0];
	for (i = 0; i < 27;) {
		roundkey[i] = A;
		EncryptRound32(B,A,i++);
		roundkey[i] = A;
		EncryptRound32(C,A,i++);
		roundkey[i] = A;
		EncryptRound32(D,A,i++);
	}
}

// Encrypt

void Block64Encrypt(uint32_t plaintext[], uint32_t ciphertext[], uint32_t roundkey[]) {
	uint32_t i;

	ciphertext[0] = plaintext[0];
	ciphertext[1] = plaintext[1];
	for (i = 0; i < 27;) {
		EncryptRound32(ciphertext[1], ciphertext[0], roundkey[i++]);
	}
}

// Decrypt
void Block32Decrypt(uint16_t plaintext[], uint16_t ciphertext[], uint16_t roundkey[]) {
	uint16_t i;

	plaintext[0] = ciphertext[0];
	plaintext[1] = ciphertext[1];
	for (i=21; i>=0;) {
		DecryptRound16(plaintext[1], plaintext[0], roundkey[i--]);
	}
}

void Block64Decrypt(uint32_t plaintext[], uint32_t ciphertext[], uint32_t roundkey[]) {
	uint32_t i;

	plaintext[0] = ciphertext[0];
	plaintext[1] = ciphertext[1];
	for (i=26; i>=0; ) {
		DecryptRound32(plaintext[1], plaintext[0], roundkey[i--]);
	}
}


// InvertIQ allows differentiating sender and receiver message.
// Usually, Gateways read messages with InvertIQ off and send messages with InvertIQ on, Nodes
// read messages with InvertIQ on and send messages with InvertIQ off.
// This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, 
// and Node never reads messages from other Node.
