
///////////////////////////////////////////////
// Libraries to include

#include <SPI.h>
#include <LoRa.h>

///////////////////////////////////////////////
// Defines

#define BUTTON_PIN 4

#define ROTL16(word, offset) (((word)<<(offset)) | (word>>(16-(offset))))
#define ROTR16(word, offset) (((word)>>(offset)) | ((word)<<(16-(offset))))
#define ROTL32(word, offset) (((word)<<(offset)) | (word>>(32-(offset))))
#define ROTR32(word, offset) (((word)>>(offset)) | ((word)<<(32-(offset))))

#define EncryptRound16(word1,word2,roundkey) (word1=(ROTR16(word1,8)+word2)^(roundkey), word2=ROTL16(word2,3)^word1)
#define EncryptRound32(word1,word2,roundkey) (word1=(ROTR32(word1,8)+word2)^(roundkey), word2=ROTL32(word2,3)^word1)

#define DecryptRound16(word1,word2,roundkey) (word2=ROTR16(word2^=word1,3), word1=ROTL16((word1^=roundkey)-word2,8))
#define DecryptRound32(word1,word2,roundkey) (word2=ROTR32(word2^=word1,3), word1=ROTL32((word1^=roundkey)-word2,8))

///////////////////////////////////////////////
// Global variables

const long frequency = 8681E5;

uint8_t rootKey[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
uint8_t secretKey[8];

uint16_t sequenceNumber = 1;      // the receiver expects a sequence number greater than 0
uint16_t deviceAddress = 0x2B8;

bool test = false;

///////////////////////////////////////////////
// Functions

void setup() {
  

  pinMode(BUTTON_PIN, INPUT);
  
	Serial.begin(115200);

  for(int i = 0; i < 20; i++) {
    Serial.println();
  }

  
  if (!test) {
    if (!LoRa.begin(frequency)) {
      Serial.println("Failed initializing LoRa connection...");
      while (1);
    }
  }

	// set coding rate
  LoRa.setCodingRate4(5);
	// set preamble length
  LoRa.setPreambleLength(10);          // change after debugging
	// enable crc
  LoRa.enableCrc();
  // define spreading factor

	// setup callback for onReceive - the callback will be triggered by an interrupt on _dio0 which calls onDio0Rise -> handleDio0Rise -> _onReceive
  
	// setup callback for onTxDone such that it can be put in receive mode af transmitting

  if (!test) {
    LoRa.idle();                          // set standby mode
    LoRa.disableInvertIQ();               // normal mode 
  }

  delay(500);

  transmitMessage(true);      // send nonce to server and derive secret key

}

void loop() {
  byte buttonState = digitalRead(BUTTON_PIN);
	// wait for 2000 msec
	if (!test) {
    /*if (waited(2000)) {
      transmitMessage(false);
    }*/
    if (buttonState == HIGH) {
      transmitMessage(false);
      delay(500);
    }
	} else {
    delay(1000);
    transmitMessage(false);
	}
}

bool waited(int interval) {
  static unsigned long prevTime = 0;
  unsigned long currentTime = millis();
  if (interval <= currentTime - prevTime) {
    prevTime = currentTime;
    return true;
  }
  return false;
}

void transmitMessage(bool firstNonce) {
	uint8_t header_b1 = deviceAddress >> 4;
	uint8_t header_b2 = (deviceAddress << 4) | (sequenceNumber >> 8);
	uint8_t header_b3 = sequenceNumber;

  uint16_t *payload;
  uint32_t mic;

  if (firstNonce) {
    payload = getFirstNonce();
    uint8_t key[8];
    for (int i = 0; i < 8; i++) {
      key[i] = rootKey[i];
    }
    
    deriveSecretKey(blakePlaceholder(payload));
    mic = getMIC(payload, key);
  } else {
    payload = getPayload();
    Serial.println("-------------------");
    Serial.print("Plaintext:       ");
    Serial.println(payload);
    payload = getCiphertext(payload);
    mic = getMIC(payload, secretKey);
  }

  uint8_t payload_b1 = payload >> 8;
  uint8_t payload_b2 = payload;
  
  uint8_t mic_b1 = mic >> 24;
  uint8_t mic_b2 = mic >> 16;
  uint8_t mic_b3 = mic >> 8;
  uint8_t mic_b4 = mic;

  Serial.print("Device Address:  ");
  Serial.println(deviceAddress);
  Serial.print("Sequence Num:    ");
  Serial.println(sequenceNumber);
  Serial.print("Ciphertext:      ");
  Serial.println(payload);
  Serial.print("Secret Key:      ");
  for (int i = 0; i < 8; i++) {
    Serial.print(secretKey[i]);
  }
  Serial.println();
  Serial.println("-------------------");

	LoRa.beginPacket();							// beginPacket(implicitHeader = 1)
	LoRa.write(header_b1);
	LoRa.write(header_b2);
	LoRa.write(header_b3);
	LoRa.write(payload_b1);
  LoRa.write(payload_b2);
	LoRa.write(mic_b1);
  LoRa.write(mic_b2);
  LoRa.write(mic_b3);
  LoRa.write(mic_b4);
	LoRa.endPacket(true);							// endPacket(true)	// true = non-blocking mode

	sequenceNumber++;
}

/* array length : 1, 2, 4, 8
 * 
 */
uint16_t * getPayload(int lengthBytes) {
  static uint16_t payload[lengthBytes];
  for (int i = 0; i < lengthBytes; i++) {
    payload[i] = random(255);
  }
	return payload;
}

uint16_t getFirstNonce() {
  static uint16_t nonce[2];
  for (int i = 0; i < 2; i++) {
    nonce[i] = random(255);
  }
  return nonce;
}

uint8_t * blakePlaceholder(uint8_t payload[]) {
  static uint8_t hashedNonce[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, payload[0], payload[1]};
  return hashedNonce;
}

/* Function that hashes the 2 byte nonce to and 8 byte nonce:
 *  
 * Input 1:  uint16_t inputNonce
 * Input 2:  uint8_t rootKey[16]        (can also use shorter portions of the rootKey if a 16 byte key is too much)
 * 
 * Output:   uint8_t outputNonce[8]
 *  
 */

/* Function that creates MIC:
 *  
 * Input 1:  uint16_t ciphertext[cipher_length]     (cipher_length can vary between 1, 2, 4, 8)
 * Input 2:  uint32_t devAddrSeqNum                 (only the least significant 24 bits should be used => if it is easier for you we can also make this variable a uint8_t array with three entries) 
 * Input 3:  uint8_t key[8]
 * 
 * Output:   uint8_t MIC[mic_length]                (mic_length can be whatever)
 * 
 */
 
uint16_t getCiphertext(uint16_t payload) {

	uint32_t msgKey = getMsgKey();
	uint16_t msgKey16 = (uint16_t)msgKey;

  uint16_t ciphertext = payload ^ msgKey16;

	return ciphertext;
}

uint32_t getMIC(uint16_t ciphertext[], uint8_t key[]) {
  // insert blake hashing

  return 1249816708;
}

uint32_t getMsgKey() {

	uint8_t plaintext[4];
  uint32_t plaintextword[1];
  uint8_t ciphertext[4];
  uint32_t ciphertextword[1];
  
	uint32_t secretkeyword[2];
	uint32_t roundkey[22];

	getAddressSeqNum(plaintext);                             // DeviceAddress + SequenceNumber (uint8 array of length 4)
	ConvertBW32(plaintext,plaintextword, 4);                  // input: uint8 array of length 4 // output: uint32 array of length 1
  ConvertBW32(secretKey,secretkeyword, 8);
  Expand64(secretkeyword, roundkey);                        // input: uint32 array of length 2 // output: uint32 array of length 22
  Block32Encrypt(plaintextword, ciphertextword, roundkey);  // input: uint32 arrays // output: uint32 array
	ConvertW32B(ciphertextword, ciphertext, 4);

	uint32_t msgKey = ((uint32_t)ciphertext[0] << 24) | ((uint32_t)ciphertext[1] << 16) | 
						        ((uint32_t)ciphertext[2] << 8) | (uint32_t)ciphertext[3];
	return msgKey;
}

void deriveSecretKey(uint8_t nonce[]) {
  uint32_t nonceword[2];
  uint8_t ciphertext[8];
  uint32_t ciphertextword[2];
  
  uint32_t rootKeyWord[4];
  uint32_t roundkey[27];
  
  ConvertBW32(nonce,nonceword,8);
  ConvertBW32(rootKey,rootKeyWord, 16);
  Expand128(rootKeyWord, roundkey);
  Block64Encrypt(nonceword, ciphertextword, roundkey);
  ConvertW32B(ciphertextword, ciphertext, 2);
  
  for (int i = 0; i < 8; i++) {
    secretKey[i] = ciphertext[i];
  }
}

// returns device address and sequence number - used as plaintext in SPECK
void getAddressSeqNum(uint8_t plaintext[]) {
  uint32_t plaintext32 = (uint32_t)(deviceAddress << 20) | (uint32_t)sequenceNumber;
  for (int i = 0; i < 4; i++) {
    plaintext[i] = (uint8_t)(plaintext32 >> 8*i);
  }
}

/*********************************************************/
/************************* SPECK *************************/
/*********************************************************/


///////////////////////////////////////////////
// Word/byte conversion

void ConvertBW32(uint8_t bytes[], uint32_t words[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount/4; i++) {
    words[i] =  (uint32_t)bytes[j] | ((uint32_t)bytes[j+1]<<8) | 
                ((uint32_t)bytes[j+2]<<16) | ((uint32_t)bytes[j+3]<<24);
    j += 4;
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

///////////////////////////////////////////////
// Key Schedule

void Expand64(uint32_t K[], uint32_t roundkey[]) {
  static uint32_t i, D=K[3], C=K[2], B=K[1], A=K[0];
  for (i = 0; i < 22;) {
    roundkey[i] = A;
    EncryptRound32(B,A,i++);
    roundkey[i] = A;
    EncryptRound32(C,A,i++);
    roundkey[i] = A;
    EncryptRound32(D,A,i++);
  }
}

void Expand128(uint32_t K[], uint32_t roundkey[]) {
	static uint32_t i, D=K[3], C=K[2], B=K[1], A=K[0];
	for (i = 0; i < 27;) {
		roundkey[i] = A;
		EncryptRound32(B,A,i++);
		roundkey[i] = A;
		EncryptRound32(C,A,i++);
		roundkey[i] = A;
		EncryptRound32(D,A,i++);
	}
}

///////////////////////////////////////////////
// Encrypt

void Block32Encrypt(uint32_t plaintext[], uint32_t ciphertext[], uint32_t roundkey[]) {
  uint16_t i;

  ciphertext[0] = plaintext[0];
  ciphertext[1] = plaintext[1];
  for (i = 0; i < 22;) {
    EncryptRound16(ciphertext[1], ciphertext[0], roundkey[i++]);
  }
}

void Block64Encrypt(uint32_t plaintext[], uint32_t ciphertext[], uint32_t roundkey[]) {
	uint16_t i;

	ciphertext[0] = plaintext[0];
	ciphertext[1] = plaintext[1];
	for (i = 0; i < 27;) {
		EncryptRound32(ciphertext[1], ciphertext[0], roundkey[i++]);
	}
}


// InvertIQ allows differentiating sender and receiver message.
// Usually, Gateways read messages with InvertIQ off and send messages with InvertIQ on, Nodes
// read messages with InvertIQ on and send messages with InvertIQ off.
// This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, 
// and Node never reads messages from other Node.
