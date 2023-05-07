/////////////////////////////////////////////
// Description
// Gateway program
// Bachelor project Comtek 6 AAU - group 618

/////////////////////////////////////////////
// Libraies to import
#include <SPI.h>
#include <LoRa.h>

//////////////////////////////////////////////
// Defines

#define ROTL16(word, offset) (((word) << (offset)) | (word >> (16 - (offset))))
#define ROTR16(word, offset) (((word) >> (offset)) | ((word) << (16 - (offset))))
#define ROTL32(word, offset) (((word) << (offset)) | (word >> (32 - (offset))))
#define ROTR32(word, offset) (((word) >> (offset)) | ((word) << (32 - (offset))))
#define ROTL64(word, offset) (((word) << (offset)) | (word >> (64 - (offset))))
#define ROTR64(word, offset) (((word) >> (offset)) | ((word) << (64 - (offset))))

#define EncryptRound16(word1, word2, roundkey) (word1 = (ROTR16(word1, 8) + word2) ^ (roundkey), word2 = ROTL16(word2, 3) ^ word1)
#define EncryptRound32(word1, word2, roundkey) (word1 = (ROTR32(word1, 8) + word2) ^ (roundkey), word2 = ROTL32(word2, 3) ^ word1)
#define EncryptRound64(word1, word2, roundkey) (word1 = (ROTR64(word1, 8) + word2) ^ (roundkey), word2 = ROTL64(word2, 3) ^ word1)

//////////////////////////////////////////////
// Variables

byte seqNum = 0;           // sequence number of outgoing messages
byte localAddress = 0xAF;  // address of the gateway
byte destination = 0xBB;   // destination of the end device

uint8_t payload_length = 2;

struct EndDevice {
  uint16_t id;
  uint8_t rootkey[16];
  uint8_t secretkey[8];
  uint16_t seqNum;
  bool activated;
};

struct EndDevice devices[5] = {};

/////////////////////////////////////////////
// Functions


uint8_t* padInitVector(uint8_t initVector[]) {
  for (int i = 4; i < (payload_length - 4); i++) {  // loop only runs if payload length is 8 or 16
    if (payload_length != i + 1) {                  // if it is not the last byte
      initVector[i] = 0x00;
    } else {
      initVector[i] = payload_length - 5;
    }
  }
}

uint8_t * getMsgKey(uint8_t initVector[], uint8_t secretKey[]) {

  uint8_t local_length = (payload_length <= 4) ? 4 : payload_length;

    if (payload_length > 4) {
    padInitVector(initVector);
  }

  uint8_t ciphertext[local_length];

  if (payload_length <= 4) {  // if 32 bits or less

    uint32_t plaintextword[1];
    uint32_t ciphertextword[1];
    uint32_t secretkeyword[2];
    uint32_t roundkey[22];

    ConvertBW32(initVector, plaintextword, 4);
    ConvertBW32(secretKey, secretkeyword, 8);
    Expand64Block64(secretkeyword, roundkey);
    Block32Encrypt(plaintextword, ciphertextword, roundkey);
    ConvertW32B(ciphertextword, ciphertext, 4);

  } else if (payload_length == 8) {  // if 64 bits blocks

    uint32_t plaintextword[2];
    uint32_t ciphertextword[2];
    uint32_t secretkeyword[4];
    uint32_t roundkey[27];

    ConvertBW32(initVector, plaintextword, 8);
    ConvertBW32(secretKey, secretkeyword, 16);
    Expand128Block64(secretkeyword, roundkey);
    Block64Encrypt(plaintextword, ciphertextword, roundkey);
    ConvertW32B(ciphertextword, ciphertext, 8);

  } else if (payload_length == 16) {  // if 128 bits blocks

    uint64_t plaintextword[2];
    uint64_t ciphertextword[2];
    uint64_t secretkeyword[2];
    uint64_t roundkey[32];

    ConvertBW64(initVector, plaintextword, 16);
    ConvertBW64(secretKey, secretkeyword, 16);
    Expand128Block128(secretkeyword, roundkey);
    Block128Encrypt(plaintextword, ciphertextword, roundkey);
    ConvertW64B(ciphertextword, ciphertext, 16);
  }

  return ciphertext;
}

void deriveSecretKey(uint8_t* nonce, int device_num) {
  /*.println("---------- Decrypting ----------");
  Serial.print("Nonce: ");
  for (int x = 0; x < 8; x++) {
    Serial.print(nonce[x]);
    Serial.print(" ");
  }
  Serial.println();*/

  uint8_t ciphertext[(payload_length <= 4) ? 8 : 16];

  if (payload_length <= 4) {  // use speck 64/128

    uint32_t nonceword[2];
    uint32_t ciphertextword[2];
    uint32_t rootKeyWord[4];
    uint32_t roundkey[27];

    ConvertBW32(nonce, nonceword, 8);
    ConvertBW32(devices[device_num].rootkey, rootKeyWord, 16);
    Expand128Block64(rootKeyWord, roundkey);
    Block64Encrypt(nonceword, ciphertextword, roundkey);
    ConvertW32B(ciphertextword, ciphertext, 2);

    for (int i = 0; i < 8; i++) {
      devices[device_num].secretkey[i] = ciphertext[i];
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
    ConvertBW64(devices[device_num].rootkey, rootKeyWord, 16);
    Expand128Block128(rootKeyWord, roundkey);
    Block128Encrypt(nonceword, ciphertextword, roundkey);
    ConvertW64B(ciphertextword, ciphertext, 16);

    for (int i = 0; i < 16; i++) {
      devices[device_num].secretkey[i] = ciphertext[i];
    }
  }

  devices[device_num].activated = true;
}

uint8_t* checkMIC(uint16_t dev_addr, uint16_t seq_num, uint8_t payload, uint8_t key[]) {
  // Do Stuff
  uint8_t mic[4] = { 0x34, 0xa2, 0x99, 0xdc };
  return mic;
}

uint8_t* blakePlaceholder(uint16_t payload) {
  static uint8_t hashedNonce[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (uint8_t)(payload >> 8), (uint8_t)payload };
  Serial.print("blakePlaceholder hashedNonce ");
  for (int i = 0; i < 8; i++) {
    Serial.print(hashedNonce[i]);
    Serial.print(" ");
  }
  Serial.println();
  return hashedNonce;
}

void getAddressSequence(uint8_t initVector[], uint16_t deviceAddress, uint16_t sequenceNum) {
  uint32_t plaintext32 = (uint32_t)(deviceAddress << 20) | (uint32_t)sequenceNum;
  for (int i = 0; i < 4; i++) {
    initVector[i] = (uint8_t)(plaintext32 >> 8 * i);
  }
}

/*************** Word/byte conversion ***************/

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

/*************** Key Schedule ***************/

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

/*************** Encrypt/decrypt ***************/

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

/////////////////////////////////////////////
// Setup environment
void setup() {
  Serial.begin(9600);

  while (!Serial)
    ;
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(868100000)) {  // EU FQ
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  // Set coding rate
  LoRa.setCodingRate4(5);
  // Set preamble length
  LoRa.setPreambleLength(10);
  // Enable CRC
  LoRa.enableCrc();
  LoRa.disableInvertIQ();
  EndDevice ed01 = {
    0x2b8,
    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
    { 0x00 },
    0x00,
    false,
  };

  devices[0] = ed01;
}

/////////////////////////////////////////////
// Main program
void loop() {
  onReceive(LoRa.parsePacket());
}


/////////////////////////////////////////////
// Function

void onReceive(int packetSize) {
  if (packetSize == 0) return;  // if there's no packet, return
  Serial.println("---------------------------------------");
  Serial.println("Packet arrived");
  uint16_t deviceAddress;
  uint16_t sequenceNum;
  uint8_t* payload;
  uint8_t* mic;

  for (int i = 0; i < packetSize; i++) {
    if (i == 0) {
      uint16_t tmp = (uint16_t)LoRa.read();
      deviceAddress = tmp << 4;
    } else if (i == 1) {
      uint16_t tmp = (uint16_t)LoRa.read();
      deviceAddress = deviceAddress | (tmp >> 4);
      sequenceNum = (tmp & (uint16_t)15) << 8;
    } else if (i == 2) {
      uint16_t tmp = (uint16_t)LoRa.read();
      sequenceNum = sequenceNum | tmp;
    } else if (i < (3 + payload_length)) {
      uint16_t tmp = (uint16_t)LoRa.read();
      payload[i - 3] = tmp;
    } else {
      uint32_t tmp = (uint32_t)LoRa.read();
      mic[i - (3 + payload_length)] = tmp;
    }
  }
  Serial.print("Device address: ");
  Serial.println(deviceAddress);
  Serial.print("Sequence number: ");
  Serial.println(sequenceNum);
  Serial.print("Received payload: ");
  for (int i = 0; i < payload_length; i++) {
    Serial.print(payload[i]);
  }
  Serial.println();
  Serial.print("MIC: ");
  for (int i = 0; i < payload_length; i++) {
    Serial.print(mic[i]);
  }
  Serial.println();

  int x;
  if (devices[0].id != 0) {
    for (x = 0; x < 5; x++) {
      if (devices[x].id == deviceAddress) {
        uint8_t key[16];
        Serial.print("Succes\n");
        if (devices[x].activated == true) {
          Serial.print("This is a payload\n");
          for (int y = 0; y < 8; y++) {
            key[y] = devices[x].secretkey[y];
          }
        } else {
          Serial.print("This is a nonce\n");
          for (int y = 0; y < 16; y++) {
            key[y] = devices[x].rootkey[y];
          }
        }

        if (devices[x].seqNum >= sequenceNum) {
          Serial.print("Dropping packet: Inconsistent seq_num\n");
          return;
        }
        if (checkMIC(deviceAddress, sequenceNum, payload, key) != mic) {
          Serial.print("Dropping packet: Incosistent mic\n");
          return;
        }

        Serial.print("Key: ");
        for (int j = 0; j < 16; j++) {
          Serial.print(key[j]);
        }
        Serial.println();
        Serial.println();

        devices[x].seqNum = sequenceNum;
        if (devices[x].activated == true) {
          // Decrypt
          Serial.println("Initiating decryption...");
          uint8_t IV[4];
          getAddressSequence(IV, deviceAddress, sequenceNum);
          uint8_t * msgKey = getMsgKey(IV, key);
          uint8_t text[payload_length];
          for (int i = 0; i < payload_length; i++) {
            text[i] = payload[i] ^ msgKey[i];
          }
          Serial.print("Received message: ");
          for (int i = 0; i < payload_length; i++) {
            Serial.print(text[i]);
          }
          Serial.println();


        } else if (devices[x].activated == false) {
          // Generate secret key
          /*uint8_t *hashedNonce;
          hashedNonce = blakePlaceholder(payload);
          Serial.println("inbetween");
          for (int j = 0; j < 8; j++) {
            Serial.print(hashedNonce[j]);
          }
          Serial.println();*/
          deriveSecretKey(blakePlaceholder(payload), x);
        } else {
          Serial.print("WTF, should not be happening!!!\n");
        }

      } else {
      }
    }
  }
}
