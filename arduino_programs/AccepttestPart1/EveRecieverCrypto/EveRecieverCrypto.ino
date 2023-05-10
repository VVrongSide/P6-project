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

#define ROTL16(word, offset) (((word)<<(offset)) | (word>>(16-(offset))))
#define ROTR16(word, offset) (((word)>>(offset)) | ((word)<<(16-(offset))))
#define ROTL32(word, offset) (((word)<<(offset)) | (word>>(32-(offset))))
#define ROTR32(word, offset) (((word)>>(offset)) | ((word)<<(32-(offset))))

#define EncryptRound16(word1,word2,roundkey) (word1=(ROTR16(word1,8)+word2)^(roundkey), word2=ROTL16(word2,3)^word1)
#define EncryptRound32(word1,word2,roundkey) (word1=(ROTR32(word1,8)+word2)^(roundkey), word2=ROTL32(word2,3)^word1)

#define DecryptRound16(word1,word2,roundkey) (word2=ROTR16(word2^=word1,3), word1=ROTL16((word1^=roundkey)-word2,8))
#define DecryptRound32(word1,word2,roundkey) (word2=ROTR32(word2^=word1,3), word1=ROTL32((word1^=roundkey)-word2,8))

//////////////////////////////////////////////
// Variables

bool replayAttack = false;
bool forgeryAttack = true;
int forgeryAttackType = 3;

uint8_t header_b1;
uint8_t header_b2;
uint8_t header_b3;
uint16_t deviceAddress;
uint16_t sequenceNum;
uint16_t payload;
uint8_t mic[4];

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


uint32_t getMsgKey(uint8_t initVector[], uint8_t secretKey[]) {

  uint8_t plaintext[4];
  uint32_t plaintextword[1];
  uint8_t ciphertext[4];
  uint32_t ciphertextword[1];

  uint32_t secretkeyword[2];
  uint32_t roundkey[22];

  ConvertBW32(initVector, plaintextword, 4);                 // input: uint8 array of length 4 // output: uint32 array of length 1
  ConvertBW32(secretKey, secretkeyword, 8);                 // input: uint8 array of length 8 // output: uint32 array of length 2
  Expand64(secretkeyword, roundkey);                        // input: uint32 array of length 2 // output: uint32 array of length 22
  Block32Encrypt(plaintextword, ciphertextword, roundkey);  // input: uint32 arrays // output: uint32 array
  ConvertW32B(ciphertextword, ciphertext, 4);

  uint32_t msgKey = ((uint32_t)ciphertext[0] << 24) | ((uint32_t)ciphertext[1] << 16) |
                    ((uint32_t)ciphertext[2] << 8) | (uint32_t)ciphertext[3];

  return msgKey;
}

void deriveSecretKey(uint8_t * nonce, int device_num) {
  uint32_t nonceword[2];
  uint8_t ciphertext[8];
  uint32_t ciphertextword[2];

  uint32_t rootKeyWord[4];
  uint32_t roundkey[27];

  ConvertBW32(nonce, nonceword, 8);
  ConvertBW32(devices[device_num].rootkey, rootKeyWord, 16);
  Expand128(rootKeyWord, roundkey);
  Block64Encrypt(nonceword, ciphertextword, roundkey);
  ConvertW32B(ciphertextword, ciphertext, 2);

  for (int i = 0; i < 8; i++) {
    devices[device_num].secretkey[i] = ciphertext[i];
  }
  devices[device_num].activated = true;
}




uint32_t checkMIC(uint16_t dev_addr, uint16_t seq_num, uint16_t payload, uint8_t key[]) {
  // Do Stuff
  return 1249816708;
}


uint8_t * blakePlaceholder(uint16_t payload) {
  static uint8_t hashedNonce[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (uint8_t)(payload >> 8), (uint8_t)payload};
  /*Serial.print("blakePlaceholder hashedNonce ");
    for (int i = 0; i < 8; i++) {
    Serial.print(hashedNonce[i]);
    Serial.print(" ");
    }
    Serial.println();*/
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
    words[i] =  (uint32_t)bytes[j] | ((uint32_t)bytes[j + 1] << 8) |
                ((uint32_t)bytes[j + 2] << 16) | ((uint32_t)bytes[j + 3] << 24);
    j += 4;
  }
}

void ConvertW32B(uint32_t words[], uint8_t bytes[], int wordcount) {
  int i, j = 0;
  for (i = 0; i < wordcount; i++) {
    bytes[j] =   (uint8_t)words[i];
    bytes[j + 1] = (uint8_t)(words[i] >> 8);
    bytes[j + 2] = (uint8_t)(words[i] >> 16);
    bytes[j + 3] = (uint8_t)(words[i] >> 24);
    j += 4;
  }
}


/*************** Key Schedule ***************/

void Expand64(uint32_t K[], uint32_t roundkey[]) {
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

void Expand128(uint32_t K[], uint32_t roundkey[]) {
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

/////////////////////////////////////////////
// Setup environment
void setup() {
  Serial.begin(115200);

  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(868100000)) {   // EU FQ
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Set coding rate
  LoRa.setCodingRate4(5);
  // Set preamble length
  LoRa.setPreambleLength(10);
  // Enable CRC
  LoRa.enableCrc();
  LoRa.disableInvertIQ();
}

/////////////////////////////////////////////
// Main program
void loop() {
  onReceive(LoRa.parsePacket());
}


/////////////////////////////////////////////
// Function

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return
  //Serial.println("---------------------------------------");
  //Serial.println("Packet arrived");
  payload = 0;

  for (int i = 0; i < packetSize; i++) {
    if (i == 0) {
      uint16_t tmp = (uint16_t)LoRa.read();
      deviceAddress = tmp << 4;
      header_b1 = (uint8_t)tmp;
    }
    else if (i == 1) {
      uint16_t tmp = (uint16_t)LoRa.read();
      deviceAddress = deviceAddress | (tmp >> 4);
      sequenceNum = (tmp & (uint16_t)15) << 8;
      header_b2 = (uint8_t)tmp;
    }
    else if (i == 2) {
      uint16_t tmp = (uint16_t)LoRa.read();
      sequenceNum = sequenceNum | tmp;
      header_b3 = (uint8_t)tmp;
    }
    else if (i < 5) {
      uint16_t tmp = (uint16_t)LoRa.read();
      payload = payload | (tmp << 8 * (4 - i));
    }
    else {
      uint8_t tmp = LoRa.read();
      mic[i - 5] = tmp;
    }
  }

  Serial.println("------------ Packet recived ------------");
  Serial.print("Device address:   ");
  Serial.print(deviceAddress);
  Serial.println("\t\t\t|\t12 bits");
  Serial.print("Sequence number:  ");
  Serial.print(sequenceNum);
  Serial.println("\t\t\t|\t12 bits");
  Serial.print("Ciphertext:       ");
  Serial.print(payload);
  Serial.print("\t\t\t|\t");
  Serial.print(sizeof(payload));
  Serial.println(" bytes");
  Serial.print("MIC:              ");
  for (int i = 0; i < 4; i++) {
    Serial.print(mic[i]);
  }
  Serial.print("\t\t|\t");
  Serial.print(sizeof(mic));
  Serial.println(" bytes");
  /*Serial.print("Secret key:       ");
  Serial.print("N/A");
  Serial.println("\t\t\t|");*/
  Serial.println("----------------------------------------");

  if (replayAttack && sequenceNum > 1) {
    delay(500);
    Serial.println("----------- Replaying packet -----------");
    transmitMessage();
  }
  else if (forgeryAttack && sequenceNum > 1) {
    delay(500);
    forgeMessage(forgeryAttackType, sequenceNum, payload);        // param: 1 = sequence number, 2 = payload, 3 = both
  }

}

void forgeMessage(uint16_t field, uint16_t seqNum, uint16_t pay) {

  if (field == 1) {                             // if forge sequence number
    Serial.println("-------- Forging sequence number --------");
    uint16_t forgedSeqNum = seqNum + 2;
    sequenceNum = forgedSeqNum;
    header_b2 &= 240;
    header_b2 ^= forgedSeqNum >> 8;
    header_b3 = forgedSeqNum;
  }
  else if (field == 2) {                        // if forge payload
    Serial.println("------------ Forging payload ------------");
    uint16_t forgedPayload = pay ^ 170;
    payload = forgedPayload;
  }
  else if (field == 3) {                        // if forge both
    Serial.println("-- Forging sequence number and payload --");
    uint16_t forgedSeqNum = seqNum + 2;
    sequenceNum = forgedSeqNum;
    header_b2 &= 240;
    header_b2 ^= forgedSeqNum >> 8;
    header_b3 = forgedSeqNum;

    uint16_t forgedPayload = pay ^ 170;
    payload = forgedPayload;
  }
  transmitMessage();
}

void transmitMessage() {

  uint8_t payload_b1 = payload >> 8;
  uint8_t payload_b2 = payload;

  Serial.print("Device address:   ");
  Serial.print(deviceAddress);
  Serial.println("\t\t\t|\t12 bits");
  Serial.print("Sequence number:  ");
  Serial.print(sequenceNum);
  Serial.println("\t\t\t|\t12 bits");
  Serial.print("Ciphertext:       ");
  Serial.print(payload);
  Serial.print("\t\t\t|\t");
  Serial.print(sizeof(payload));
  Serial.println(" bytes");
  Serial.print("MIC:              ");
  for (int i = 0; i < 4; i++) {
    Serial.print(mic[i]);
  }
  Serial.print("\t\t|\t");
  Serial.print(sizeof(mic));
  Serial.println(" bytes");
  Serial.println("----------------------------------------");
  Serial.println();

  LoRa.beginPacket();              // beginPacket(implicitHeader = 1)
  LoRa.write(header_b1);
  LoRa.write(header_b2);
  LoRa.write(header_b3);
  LoRa.write(payload_b1);
  LoRa.write(payload_b2);
  LoRa.write(mic[0]);
  LoRa.write(mic[1]);
  LoRa.write(mic[2]);
  LoRa.write(mic[3]);
  LoRa.endPacket(true);             // endPacket(true)  // true = non-blocking mode

}
