
///////////////////////////////////////////////
// Libraries to include

#include <SPI.h>
#include <LoRa.h>

///////////////////////////////////////////////
// Defines

#define BUTTON_PIN 4
#define PAYLOAD_LEN 2

#define ROTL16(word, offset) (((word) << (offset)) | (word >> (16 - (offset))))
#define ROTR16(word, offset) (((word) >> (offset)) | ((word) << (16 - (offset))))
#define ROTL32(word, offset) (((word) << (offset)) | (word >> (32 - (offset))))
#define ROTR32(word, offset) (((word) >> (offset)) | ((word) << (32 - (offset))))
#define ROTL64(word, offset) (((word) << (offset)) | (word >> (64 - (offset))))
#define ROTR64(word, offset) (((word) >> (offset)) | ((word) << (64 - (offset))))

#define EncryptRound16(word1, word2, roundkey) (word1 = (ROTR16(word1, 8) + word2) ^ (roundkey), word2 = ROTL16(word2, 3) ^ word1)
#define EncryptRound32(word1, word2, roundkey) (word1 = (ROTR32(word1, 8) + word2) ^ (roundkey), word2 = ROTL32(word2, 3) ^ word1)
#define EncryptRound64(word1, word2, roundkey) (word1 = (ROTR64(word1, 8) + word2) ^ (roundkey), word2 = ROTL64(word2, 3) ^ word1)

///////////////////////////////////////////////
// Global variables

const long frequency = 8681E5;

uint8_t rootKey[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
uint8_t secretKey[8];

uint16_t sequenceNumber = 1;  // the receiver expects a sequence number greater than 0
uint16_t deviceAddress = 0x2B8;

bool test = false;

///////////////////////////////////////////////
// Functions

void setup() {


  pinMode(BUTTON_PIN, INPUT);

  Serial.begin(115200);

  for (int i = 0; i < 20; i++) {
    Serial.println();
  }


  if (!test) {
    if (!LoRa.begin(frequency)) {
      Serial.println("Failed initializing LoRa connection...");
      while (1)
        ;
    }
  }

  // set coding rate
  LoRa.setCodingRate4(5);
  // set preamble length
  LoRa.setPreambleLength(10);  // change after debugging
                               // enable crc
  LoRa.enableCrc();
  // define spreading factor

  // setup callback for onReceive - the callback will be triggered by an interrupt on _dio0 which calls onDio0Rise -> handleDio0Rise -> _onReceive

  // setup callback for onTxDone such that it can be put in receive mode af transmitting

  if (!test) {
    LoRa.idle();             // set standby mode
    LoRa.disableInvertIQ();  // normal mode
  }

  delay(500);

  transmitMessage(true, PAYLOAD_LEN);  // send nonce to server and derive secret key
}

void loop() {
  byte buttonState = digitalRead(BUTTON_PIN);
  // wait for 2000 msec
  if (!test) {
    /*if (waited(2000)) {
      transmitMessage(false);
    }*/
    if (buttonState == HIGH) {
      transmitMessage(false, PAYLOAD_LEN);
      delay(500);
    }
  } else {
    delay(1000);
    transmitMessage(false, PAYLOAD_LEN);
  }
}

/*

----- TODO -----

 - Replay attack function
 - Forgery functionality?
 - Receiver window
 - Generate new key + scenario where receiver window is disabled 
 - Blake implementation
 Ø Padding

*/

void transmitMessage(bool firstNonce, uint8_t payload_length) {
  uint8_t header_b1 = deviceAddress >> 4;
  uint8_t header_b2 = (deviceAddress << 4) | (sequenceNumber >> 8);
  uint8_t header_b3 = sequenceNumber;

  uint8_t *payload;
  uint8_t *mic;

  if (firstNonce) {
    payload = getFirstNonce(payload_length);
    uint8_t key[8];
    for (int i = 0; i < 8; i++) {
      key[i] = rootKey[i];
    }

    deriveSecretKey(blakePlaceholder(payload), payload_length);
    mic = getMIC(payload, key);
  } else {
    payload = getPayload(payload_length);
    Serial.println("-------------------");
    Serial.print("Plaintext:       ");
    for (int i = 0; i < payload_length; i++) {
      Serial.print(payload[i]);
    }
    payload = getCiphertext(payload, payload_length);
    mic = getMIC(payload, secretKey);
  }

  Serial.print("Device Address:  ");
  Serial.println(deviceAddress);
  Serial.print("Sequence Num:    ");
  Serial.println(sequenceNumber);
  Serial.print("Ciphertext:      ");
  for (int i = 0; i < payload_length; i++) {
    Serial.print(payload[i]);
  }
  Serial.println();
  Serial.print("Secret Key:      ");
  for (int i = 0; i < 8; i++) {
    Serial.print(secretKey[i]);
  }
  Serial.println();
  Serial.println("-------------------");

  LoRa.beginPacket();  // beginPacket(implicitHeader = 1)
  LoRa.write(header_b1);
  LoRa.write(header_b2);
  LoRa.write(header_b3);
  for (int i = 0; i < payload_length; i++) {
    LoRa.write(payload[i]);
  }
  LoRa.write(mic[0]);
  LoRa.write(mic[1]);
  LoRa.write(mic[2]);
  LoRa.write(mic[3]);
  LoRa.endPacket(true);  // endPacket(true)	// true = non-blocking mode

  sequenceNumber++;
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

uint8_t *getPayload(uint8_t payload_length) {
  static uint8_t payload[PAYLOAD_LEN];
  for (int i = 0; i < payload_length; i++) {
    payload[i] = random(255);
  }
  return payload;
}

uint8_t *getFirstNonce(uint8_t payload_length) {
  static uint8_t nonce[PAYLOAD_LEN];
  for (int i = 0; i < payload_length; i++) {
    nonce[i] = random(255);
  }
  return nonce;
}

uint8_t *blakePlaceholder(uint8_t payload[]) {
  static uint8_t hashedNonce[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, payload[0], payload[1] };
  return hashedNonce;
}

/*
 *  1. Generates uint16_t msgKey[msgKey_length],    msgKey_length = 2,4,8
 *  2. XORs msgKey and payload
 *  3. Returns uint16_t ciphertext[cipher_length],  cipher_length = 1,2,4
 */
uint16_t getCiphertext(uint8_t payload[], uint8_t payload_length) {

  uint8_t *msgKey = getMsgKey(payload_length);
  uint16_t ciphertext[payload_length / 2];

  for (int i = 0; i < payload_length / 2; i++) {
    ciphertext[i] = (uint16_t)((payload[i * 2] ^ msgKey[i * 2]) << 8) | (uint16_t)(payload[(i * 2) + 1] ^ msgKey[(i * 2) + 1]);
  }

  return ciphertext;
}

uint8_t * getMIC(uint8_t ciphertext[], uint8_t key[]) {
  // insert blake hashing
  uint8_t mic[4] = {0x34, 0xa2, 0x99, 0xdc};
  return mic;
}

uint8_t getMsgKey(uint8_t payload_length) {

  uint8_t local_length = (payload_length <= 4) ? 4 : payload_length;

  uint8_t plaintext[local_length];
  uint8_t ciphertext[local_length];
  getAddressSeqNum(plaintext, local_length);

  if (payload_length <= 4) {  // if 32 bits or less

    uint32_t plaintextword[1];
    uint32_t ciphertextword[1];
    uint32_t secretkeyword[2];
    uint32_t roundkey[22];

    ConvertBW32(plaintext, plaintextword, 4);
    ConvertBW32(secretKey, secretkeyword, 8);
    Expand64Block64(secretkeyword, roundkey);
    Block32Encrypt(plaintextword, ciphertextword, roundkey);
    ConvertW32B(ciphertextword, ciphertext, 4);

  } else if (payload_length == 8) {  // if 64 bits blocks

    uint32_t plaintextword[2];
    uint32_t ciphertextword[2];
    uint32_t secretkeyword[4];
    uint32_t roundkey[27];

    ConvertBW32(plaintext, plaintextword, 8);
    ConvertBW32(secretKey, secretkeyword, 16);
    Expand128Block64(secretkeyword, roundkey);
    Block64Encrypt(plaintextword, ciphertextword, roundkey);
    ConvertW32B(ciphertextword, ciphertext, 8);

  } else if (payload_length == 16) {  // if 128 bits blocks

    uint64_t plaintextword[2];
    uint64_t ciphertextword[2];
    uint64_t secretkeyword[2];
    uint64_t roundkey[32];

    ConvertBW64(plaintext, plaintextword, 16);
    ConvertBW64(secretKey, secretkeyword, 16);
    Expand128Block128(secretkeyword, roundkey);
    Block128Encrypt(plaintextword, ciphertextword, roundkey);
    ConvertW64B(ciphertextword, ciphertext, 16);
  }

  return ciphertext;
}

void deriveSecretKey(uint8_t nonce[], uint8_t payload_length) {

  uint8_t ciphertext[(payload_length <= 4) ? 8 : 16];

  if (payload_length <= 4) {  // use speck 64/128

    uint32_t nonceword[2];
    uint32_t ciphertextword[2];
    uint32_t rootKeyWord[4];
    uint32_t roundkey[27];

    ConvertBW32(nonce, nonceword, 8);
    ConvertBW32(rootKey, rootKeyWord, 16);
    Expand128Block64(rootKeyWord, roundkey);
    Block64Encrypt(nonceword, ciphertextword, roundkey);
    ConvertW32B(ciphertextword, ciphertext, 2);

    for (int i = 0; i < 8; i++) {
      secretKey[i] = ciphertext[i];
    }

  } else {  // use speck 128/128

    uint64_t nonceword[2];
    uint64_t ciphertextword[2];
    uint64_t rootKeyWord[2];
    uint64_t roundkey[32];
    uint8_t doubleNonce[16];  // we duplicate the 8 byte nonce to become a 16 byte nonce
    for (int i = 0; i < 16; i++) {
      doubleNonce[i] = nonce[i % 8];
    }

    ConvertBW64(doubleNonce, nonceword, 16);
    ConvertBW64(rootKey, rootKeyWord, 16);
    Expand128Block128(rootKeyWord, roundkey);
    Block128Encrypt(nonceword, ciphertextword, roundkey);
    ConvertW64B(ciphertextword, ciphertext, 16);

    for (int i = 0; i < 16; i++) {
      secretKey[i] = ciphertext[i];
    }
  }
}

// returns device address and sequence number - used as plaintext in SPECK
void getAddressSeqNum(uint8_t plaintext[], uint8_t payload_length) {
  uint32_t plaintext32 = (uint32_t)(deviceAddress << 20) | (uint32_t)sequenceNumber;
  for (int i = 0; i < 4; i++) {
    plaintext[i] = (uint8_t)(plaintext32 >> 8 * i);
  }

  for (int i = 4; i < (payload_length - 4); i++) {      // loop only runs if payload length is 8 or 16
    if (payload_length != i+1) {                        // if it is not the last byte
      plaintext[i] = 0x00;
    }
    else {
      plaintext[i] = payload_length - 5;
    }
  }
}

/*********************************************************/
/************************* SPECK *************************/
/*********************************************************/


///////////////////////////////////////////////
// Word/byte conversion

void ConvertBW32(uint8_t bytes[], uint32_t words[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount / 4; i++) {
    words[i] = (uint32_t)bytes[j] | ((uint32_t)bytes[j + 1] << 8) | ((uint32_t)bytes[j + 2] << 16) | ((uint32_t)bytes[j + 3] << 24);
    j += 4;
  }
}

void ConvertW32B(uint32_t words[], uint8_t bytes[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount; i++) {
    bytes[j] = (uint8_t)words[i];
    bytes[j + 1] = (uint8_t)(words[i] >> 8);
    bytes[j + 2] = (uint8_t)(words[i] >> 16);
    bytes[j + 3] = (uint8_t)(words[i] >> 24);
    j += 4;
  }
}

void ConvertW64B(uint64_t words[], uint8_t bytes[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount; i++) {
    bytes[j] = (uint8_t)words[i];
    bytes[j + 1] = (uint8_t)(words[i] >> 8);
    bytes[j + 2] = (uint8_t)(words[i] >> 16);
    bytes[j + 3] = (uint8_t)(words[i] >> 24);
    bytes[j + 4] = (uint8_t)(words[i] >> 32);
    bytes[j + 5] = (uint8_t)(words[i] >> 40);
    bytes[j + 6] = (uint8_t)(words[i] >> 48);
    bytes[j + 7] = (uint8_t)(words[i] >> 56);
    j += 8;
  }
}

void ConvertBW64(uint8_t bytes[], uint64_t words[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount / 8; i++) {
    words[i] = (uint8_t)bytes[j] | ((uint8_t)bytes[j + 1] << 8) | ((uint8_t)bytes[j + 2] << 16) | ((uint8_t)bytes[j + 3] << 24) | ((uint8_t)bytes[j + 4] << 32) | ((uint8_t)bytes[j + 5] << 40) | ((uint8_t)bytes[j + 6] << 48) | ((uint8_t)bytes[j + 7] << 56);
    j += 8;
  }
}

///////////////////////////////////////////////
// Key Schedule

void Expand64Block64(uint32_t K[], uint32_t roundkey[]) {
  static uint32_t i, D = K[3], C = K[2], B = K[1], A = K[0];
  for (i = 0; i < 22;) {
    roundkey[i] = A;
    EncryptRound32(B, A, i++);
    roundkey[i] = A;
    EncryptRound32(C, A, i++);
    roundkey[i] = A;
    EncryptRound32(D, A, i++);
  }
}

void Expand128Block64(uint32_t K[], uint32_t roundkey[]) {
  static uint32_t i, D = K[3], C = K[2], B = K[1], A = K[0];
  for (i = 0; i < 27;) {
    roundkey[i] = A;
    EncryptRound32(B, A, i++);
    roundkey[i] = A;
    EncryptRound32(C, A, i++);
    roundkey[i] = A;
    EncryptRound32(D, A, i++);
  }
}

void Expand128Block128(uint64_t K[], uint64_t roundkey[]) {
  static uint32_t i, B = K[1], A = K[0];

  for (i = 0; i < 31;) {
    roundkey[i] = A;
    EncryptRound64(B, A, i++);
  }
  roundkey[i] = A;
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

void Block128Encrypt(uint64_t plaintext[], uint64_t ciphertext[], uint64_t roundkey[]) {
  uint16_t i;

  ciphertext[0] = plaintext[0];
  ciphertext[1] = plaintext[1];
  for (i = 0; i < 32;) {
    EncryptRound64(ciphertext[1], ciphertext[0], roundkey[i++]);
  }
}

// InvertIQ allows differentiating sender and receiver message.
// Usually, Gateways read messages with InvertIQ off and send messages with InvertIQ on, Nodes
// read messages with InvertIQ on and send messages with InvertIQ off.
// This way a Gateway only reads messages from Nodes and never reads messages from other Gateway,
// and Node never reads messages from other Node.
