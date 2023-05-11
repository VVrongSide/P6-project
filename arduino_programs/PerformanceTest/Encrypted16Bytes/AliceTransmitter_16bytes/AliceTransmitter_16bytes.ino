
///////////////////////////////////////////////
// Libraries to include

#include <SPI.h>
#include <LoRa.h>
#include <LowPower.h>

///////////////////////////////////////////////
// Defines

#define DATA_PROCESS_PIN 3    // data processing pin number
#define DATA_TRANSMIT_PIN 4   // data transmitting pin number
#define DATA_RECEIVE_PIN 6    // data receive pin number

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

uint8_t rootKey[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
uint8_t secretKey[8];

uint16_t sequenceNumber = 1;      // the receiver expects a sequence number greater than 0
uint16_t deviceAddress = 0x2B8;

bool test = false;


/*********************************************************/
/************************* BLAKE *************************/
/*********************************************************/

// Little-endian byte access.
typedef struct blake2s_ctx {
  uint8_t b[64];                      // input buffer
  uint32_t h[8];                      // chained state
  uint32_t t[2];                      // total number of bytes
  size_t c;                           // pointer for b[]
  size_t outlen;                      // digest size
};

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 32 gives the digest size in bytes.
//      Secret key (also <= 32 bytes) is optional (keylen = 0).
int blake2s_init(blake2s_ctx *ctx, size_t outlen,
                 const void *key, size_t keylen);    // secret key

// Add "inlen" bytes from "in" into the hash.
void blake2s_update(blake2s_ctx *ctx,   // context
                    const void *in, size_t inlen);      // data to be hashed

// Generate the message digest (size given in init).
//      Result placed in "out".
void blake2s_final(blake2s_ctx *ctx, void *out);

// All-in-one convenience function.
int blake2s(void *out, size_t outlen,   // return buffer for digest
            const void *key, size_t keylen,     // optional secret key
            const void *in, size_t inlen);      // data to be hashed


#define B2S_GET32(p)                            \
  (((uint32_t) ((uint8_t *) (p))[0]) ^        \
   (((uint32_t) ((uint8_t *) (p))[1]) << 8) ^  \
   (((uint32_t) ((uint8_t *) (p))[2]) << 16) ^ \
   (((uint32_t) ((uint8_t *) (p))[3]) << 24))

// Mixing function G.

#define B2S_G(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = ROTR32(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR32(v[b] ^ v[c], 12); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = ROTR32(v[d] ^ v[a], 8);  \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR32(v[b] ^ v[c], 7); }

// Initialization Vector.

static const uint32_t blake2s_iv[8] =
{
  0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
  0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

// Compression function. "last" flag indicates last block.

static void blake2s_compress(struct blake2s_ctx *ctx, int last) {

  const uint8_t sigma[10][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
    { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
    { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
    { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
    { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
    { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
    { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
    { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
  };
  int i;
  uint32_t v[16], m[16];

  for (i = 0; i < 8; i++) {           // init work variables
    v[i] = ctx->h[i];
    v[i + 8] = blake2s_iv[i];
  }

  v[12] ^= ctx->t[0];                 // low 32 bits of offset
  v[13] ^= ctx->t[1];                 // high 32 bits
  if (last)                           // last block flag set ?
    v[14] = ~v[14];

  for (i = 0; i < 16; i++)            // get little-endian words
    m[i] = B2S_GET32(&ctx->b[4 * i]);

  for (i = 0; i < 10; i++) {          // ten rounds
    B2S_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
    B2S_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
    B2S_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
    B2S_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
    B2S_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
    B2S_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
    B2S_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
    B2S_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
  }

  for ( i = 0; i < 8; ++i )
    ctx->h[i] ^= v[i] ^ v[i + 8];
}

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 32 gives the digest size in bytes.
//      Secret key (also <= 32 bytes) is optional (keylen = 0).

int blake2s_init(blake2s_ctx *ctx, size_t outlen,
                 const void *key, size_t keylen)     // (keylen=0: no key)
{
  size_t i;

  if (outlen == 0 || outlen > 32 || keylen > 32)
    return -1;                      // illegal parameters

  for (i = 0; i < 8; i++)             // state, "param block"
    ctx->h[i] = blake2s_iv[i];
  ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

  ctx->t[0] = 0;                      // input count low word
  ctx->t[1] = 0;                      // input count high word
  ctx->c = 0;                         // pointer within buffer
  ctx->outlen = outlen;

  for (i = keylen; i < 64; i++)       // zero input block
    ctx->b[i] = 0;
  if (keylen > 0) {
    blake2s_update(ctx, key, keylen);
    ctx->c = 64;                    // at the end
  }

  return 0;
}

// Add "inlen" bytes from "in" into the hash.

void blake2s_update(blake2s_ctx *ctx,
                    const void *in, size_t inlen)       // data bytes
{
  size_t i;

  for (i = 0; i < inlen; i++) {
    if (ctx->c == 64) {             // buffer full ?
      ctx->t[0] += ctx->c;        // add counters
      if (ctx->t[0] < ctx->c)     // carry overflow ?
        ctx->t[1]++;            // high word
      blake2s_compress(ctx, 0);   // compress (not last)
      ctx->c = 0;                 // counter to zero
    }
    ctx->b[ctx->c++] = ((const uint8_t *) in)[i];
  }
}

// Generate the message digest (size given in init).
//      Result placed in "out".

void blake2s_final(blake2s_ctx *ctx, void *out)
{
  size_t i;

  ctx->t[0] += ctx->c;                // mark last block offset
  if (ctx->t[0] < ctx->c)             // carry overflow
    ctx->t[1]++;                    // high word

  while (ctx->c < 64)                 // fill up with zeros
    ctx->b[ctx->c++] = 0;
  blake2s_compress(ctx, 1);           // final block flag = 1

  // little endian convert and store
  for (i = 0; i < ctx->outlen; i++) {
    ((uint8_t *) out)[i] =
      (ctx->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
  }
}

int blake2s(void *out, size_t outlen, const void *key, size_t keylen, const void *in, size_t inlen) {

  blake2s_ctx ctx;

  if (blake2s_init(&ctx, outlen, key, keylen))
    return -1;
  blake2s_update(&ctx, in, inlen);
  blake2s_final(&ctx, out);

  return 0;
}


///////////////////////////////////////////////
// Functions
bool waited(int interval) {
  static unsigned long prevTime = 0;
  unsigned long currentTime = millis();
  if (interval <= currentTime - prevTime) {
    prevTime = currentTime;
    return true;
  }
  return false;
}

void onTxDone() {
  //Serial.println("txDone:");
  digitalWrite(DATA_TRANSMIT_PIN, LOW);                                                   // [STOP] Data transmission
  digitalWrite(DATA_RECEIVE_PIN, HIGH);                                                   // [START] Wait for incoming data
  LoRa.enableInvertIQ();
  LoRa.receive();

}

void onReceive(int packetSize) {
  // read packet
  for (int i = 0; i < packetSize; i++) {
    Serial.print((char)LoRa.read());
  }
  Serial.println();
}

void transmitMessage(bool firstNonce) {
  LoRa.disableInvertIQ();
  LoRa.idle();

  digitalWrite(DATA_PROCESS_PIN, HIGH);                                   // [START] Data processing

  uint8_t header_b1 = deviceAddress >> 4;
  uint8_t header_b2 = (deviceAddress << 4) | (sequenceNumber >> 8);
  uint8_t header_b3 = sequenceNumber;

  uint16_t payload[8];
  uint8_t mic[4];

  /*Serial.println("---------------------- Before encryption ----------------------");
    Serial.print("Device Address:  ");
    Serial.print(deviceAddress);
    Serial.print("\t\t\t\t\t\t | ");
    Serial.println("12 bits");
    Serial.print("Sequence Num:    ");
    Serial.print(sequenceNumber);
    Serial.print("\t\t\t\t\t\t | ");
    Serial.println("12 bits");
  */
  if (firstNonce) {
    payload[0] = getFirstNonce();

    /*   Serial.print("Nonce:           ");
       Serial.print(payload[0]);
       Serial.print("\t\t\t\t\t\t | ");
       Serial.print(sizeof(payload[0]));
       Serial.println(" bytes");*/

    uint8_t longNonce[16];
    uint8_t nonceInput[2] = {(uint8_t)(payload[0] >> 8), (uint8_t)payload[0]};
    blake2s(&longNonce, 16, rootKey, 16, nonceInput, 2);
    deriveSecretKey(longNonce);
    //deriveSecretKey(blakePlaceholder(payload));

    uint8_t micInput[5] = {header_b1, header_b2, header_b3, (uint8_t)payload >> 8, (uint8_t)payload};
    blake2s(mic, 4, rootKey, 16, micInput, 5);
    //mic = getMIC(payload, key);
  } else {
    uint16_t tempPayload[8];
    getPayload(tempPayload);
    /*Serial.print("Plaintext:       ");
      for (int i = 0; i < 8; i++) {
      Serial.print(tempPayload[i]);
      }
      Serial.print("\t | ");
      Serial.print(sizeof(tempPayload));
      Serial.println(" bytes");*/
    getCiphertext(tempPayload, payload);

    uint8_t micInput[5] = {header_b1, header_b2, header_b3, (uint8_t)payload >> 8, (uint8_t)payload};
    blake2s(mic, 4, secretKey, 16, micInput, 5);
  }

  //Serial.println("---------------------------------------------------------------");

  uint8_t payload_b1 = payload[0] >> 8;
  uint8_t payload_b2 = payload[0];
  uint8_t payload_b3 = payload[1] >> 8;
  uint8_t payload_b4 = payload[1];
  uint8_t payload_b5 = payload[2] >> 8;
  uint8_t payload_b6 = payload[2];
  uint8_t payload_b7 = payload[3] >> 8;
  uint8_t payload_b8 = payload[3];
  uint8_t payload_b9 = payload[4] >> 8;
  uint8_t payload_b10 = payload[4];
  uint8_t payload_b11 = payload[5] >> 8;
  uint8_t payload_b12 = payload[5];
  uint8_t payload_b13 = payload[6] >> 8;
  uint8_t payload_b14 = payload[6];
  uint8_t payload_b15 = payload[7] >> 8;
  uint8_t payload_b16 = payload[7];

  digitalWrite(DATA_PROCESS_PIN, LOW);                                      // [STOP] Data processing

  /*Serial.println("----------------------- After encryption ----------------------");
    Serial.print("Device Address:  ");
    Serial.print(deviceAddress);
    Serial.print("\t\t\t\t\t\t | ");
    Serial.println("12 bits");
    Serial.print("Sequence Num:    ");
    Serial.print(sequenceNumber);
    Serial.print("\t\t\t\t\t\t | ");
    Serial.println("12 bits");
    if (sequenceNumber > 1) {
    Serial.print("Ciphertext:      ");
    Serial.print(payload[0]);
    Serial.print(payload[1]);
    Serial.print(payload[2]);
    Serial.print(payload[3]);
    Serial.print(payload[4]);
    Serial.print(payload[5]);
    Serial.print(payload[6]);
    Serial.print(payload[7]);
    Serial.print("\t\t | ");
    Serial.print(sizeof(payload));
    Serial.println(" bytes");
    }
    Serial.print("MIC:             ");
    for (int i = 0; i < 4; i++) {
    Serial.print(mic[i]);
    }
    Serial.print("\t\t\t\t\t | ");
    Serial.print(sizeof(mic));
    Serial.println(" bytes");
    Serial.print("Secret Key:      ");
    for (int i = 0; i < 16; i++) {
    Serial.print(secretKey[i]);
    }
    Serial.println();
    Serial.println("---------------------------------------------------------------");
    Serial.println();*/

  digitalWrite(DATA_TRANSMIT_PIN, HIGH);                                            // [START] Data transmit

  LoRa.beginPacket();							// beginPacket(implicitHeader = 1)
  LoRa.write(header_b1);
  LoRa.write(header_b2);
  LoRa.write(header_b3);
  LoRa.write(payload_b1);
  LoRa.write(payload_b2);
  LoRa.write(payload_b3);
  LoRa.write(payload_b4);
  LoRa.write(payload_b5);
  LoRa.write(payload_b6);
  LoRa.write(payload_b7);
  LoRa.write(payload_b8);
  LoRa.write(payload_b9);
  LoRa.write(payload_b10);
  LoRa.write(payload_b11);
  LoRa.write(payload_b12);
  LoRa.write(payload_b13);
  LoRa.write(payload_b14);
  LoRa.write(payload_b15);
  LoRa.write(payload_b16);
  LoRa.write(mic[0]);
  LoRa.write(mic[1]);
  LoRa.write(mic[2]);
  LoRa.write(mic[3]);
  LoRa.endPacket(true);							// endPacket(true)	// true = non-blocking mode

  sequenceNumber++;
}

void getPayload(uint16_t payload[]) {
  for (int i = 0; i < 4; i++) {
    payload[i] = 43690;                     // equivalent to 1010101010101010
  }
}

uint16_t getFirstNonce() {
  return 42069;
}


void getCiphertext(uint16_t payload[], uint16_t ciphertext[]) {

  uint64_t msgKey[2];

  getMsgKey(msgKey);

  ciphertext[0] = payload[0] ^ (uint16_t)(msgKey[0] >> 48);
  ciphertext[1] = payload[1] ^ (uint16_t)(msgKey[0] >> 32);
  ciphertext[2] = payload[2] ^ (uint16_t)(msgKey[0] >> 16);
  ciphertext[3] = payload[3] ^ (uint16_t)msgKey[0];
  ciphertext[4] = payload[4] ^ (uint16_t)(msgKey[1] >> 48);
  ciphertext[5] = payload[5] ^ (uint16_t)(msgKey[1] >> 32);
  ciphertext[6] = payload[6] ^ (uint16_t)(msgKey[1] >> 16);
  ciphertext[7] = payload[7] ^ (uint16_t)msgKey[1];
}

void getMsgKey(uint64_t msgKey[]) {

  uint8_t plaintext[16];
  uint64_t plaintextword[2];
  uint8_t ciphertext[16];
  uint64_t ciphertextword[2];

  uint64_t secretkeyword[2];
  uint64_t roundkey[32];

  getAddressSeqNum(plaintext);                             // DeviceAddress + SequenceNumber (uint8 array of length 4)
  ConvertBW64(plaintext, plaintextword, 16);                 // input: uint8 array of length 4 // output: uint32 array of length 1
  ConvertBW64(secretKey, secretkeyword, 16);
  Expand128Block128(secretkeyword, roundkey);                        // input: uint32 array of length 2 // output: uint32 array of length 22
  Block128Encrypt(plaintextword, ciphertextword, roundkey);  // input: uint32 arrays // output: uint32 array
  ConvertW64B(ciphertextword, ciphertext, 2);

  msgKey[0] = ((uint64_t)ciphertext[0] << 56) | ((uint64_t)ciphertext[1] << 48) |
              ((uint64_t)ciphertext[2] << 40) | ((uint64_t)ciphertext[3] << 32) |
              ((uint64_t)ciphertext[4] << 24) | ((uint64_t)ciphertext[5] << 16) |
              ((uint64_t)ciphertext[6] << 8) | ((uint64_t)ciphertext[7] << 0);

  msgKey[1] = ((uint64_t)ciphertext[8] << 120) | ((uint64_t)ciphertext[9] << 112) |
              ((uint64_t)ciphertext[10] << 104) | ((uint64_t)ciphertext[11] << 96) |
              ((uint64_t)ciphertext[12] << 88) | ((uint64_t)ciphertext[13] << 80) |
              ((uint64_t)ciphertext[14] << 72) | ((uint64_t)ciphertext[15] << 64);
}

void deriveSecretKey(uint8_t nonce[]) {
  uint64_t nonceword[2];
  uint8_t ciphertext[16];
  uint64_t ciphertextword[2];

  uint64_t rootKeyWord[2];
  uint64_t roundkey[32];

  ConvertBW64(nonce, nonceword, 16);
  ConvertBW64(rootKey, rootKeyWord, 16);
  Expand128Block128(rootKeyWord, roundkey);
  Block128Encrypt(nonceword, ciphertextword, roundkey);
  ConvertW64B(ciphertextword, ciphertext, 2);

  for (int i = 0; i < 16; i++) {
    secretKey[i] = ciphertext[i];
  }
}

// returns device address and sequence number - used as plaintext in SPECK
void getAddressSeqNum(uint8_t plaintext[]) {
  uint32_t plaintext32 = (uint32_t)(deviceAddress << 20) | (uint32_t)sequenceNumber;
  for (int i = 0; i < 4; i++) {
    plaintext[i] = (uint8_t)(plaintext32 >> 8 * i);
  }
  for (int i = 4; i < 16; i++) {
    plaintext[i] = 0x00;
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


void setup() {
  pinMode(DATA_PROCESS_PIN, OUTPUT);
  pinMode(DATA_TRANSMIT_PIN, OUTPUT);
  pinMode(DATA_RECEIVE_PIN, OUTPUT);

  Serial.begin(115200);

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
  //LoRa.onReceive(onReceive);
  // setup callback for onTxDone such that it can be put in receive mode af transmitting
  LoRa.onTxDone(onTxDone);

  LoRa.onReceive(onReceive);

  if (!test) {
    LoRa.idle();                          // set standby mode
    LoRa.disableInvertIQ();               // normal mode
  }

  delay(500);

  transmitMessage(true);      // send nonce to server and derive secret key

}

void loop() {
  digitalWrite(DATA_PROCESS_PIN, HIGH);                                                                   // [START] Data processing
  transmitMessage(false);
  delay(200);
  digitalWrite(DATA_RECEIVE_PIN, LOW);                                                                    // [STOP] Wait for incoming data
 
  LoRa.disableInvertIQ();
  LoRa.idle();
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}
