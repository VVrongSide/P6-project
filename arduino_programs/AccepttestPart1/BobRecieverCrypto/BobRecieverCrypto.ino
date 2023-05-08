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

byte seqNum = 0;              // sequence number of outgoing messages
byte localAddress = 0xAF;     // address of the gateway
byte destination = 0xBB;      // destination of the end device

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
  /*Serial.println("---------- Decrypting ----------");
    Serial.print("Nonce: ");
    for (int x=0; x < 8; x++) {
    Serial.print(nonce[x]);
    Serial.print(" ");
    }
    Serial.println();*/

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
  EndDevice ed01 = {
    0x2b8,
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
    {0x00},
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
  if (packetSize == 0) return;          // if there's no packet, return
  //Serial.println("---------------------------------------");
  uint8_t headerFields[3];
  uint16_t deviceAddress;
  uint16_t sequenceNum;
  uint16_t payload = 0;
  uint8_t mic[4];

  for (int i = 0; i < packetSize; i++) {
    if (i == 0) {
      uint16_t tmp = (uint16_t)LoRa.read();
      deviceAddress = tmp << 4;
      headerFields[i] = (uint8_t)tmp;
    }
    else if (i == 1) {
      uint16_t tmp = (uint16_t)LoRa.read();
      deviceAddress = deviceAddress | (tmp >> 4);
      sequenceNum = (tmp & (uint16_t)15) << 8;
      headerFields[i] = (uint8_t)tmp;
    }
    else if (i == 2) {
      uint16_t tmp = (uint16_t)LoRa.read();
      sequenceNum = sequenceNum | tmp;
      headerFields[i] = (uint8_t)tmp;
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

  int x;
  if (devices[0].id != 0) {
    for (x = 0; x < 5; x++) {
      if (devices[x].id == deviceAddress) {
        uint8_t keyLength = 16;
        uint8_t key[16];
        //Serial.print("Succes\n");
        if (devices[x].activated == true) {
          keyLength = 8;
          //Serial.println("SecretKEY");
          for (int y = 0; y < 8; y++) {
            key[y] = devices[x].secretkey[y];
            //Serial.print(devices[x].secretkey[y]);
          }
          //Serial.println();
        } else {
          //Serial.print("This is a nonce\n");
          for (int y = 0; y < 16; y++) {
            key[y] = devices[x].rootkey[y];
          }
        }

        if (devices[x].seqNum >= sequenceNum) {
          //Serial.print("Dropping packet: Inconsistent seq_num\n");
          return;
        }

        uint8_t micInput[5] = {headerFields[0], headerFields[1], headerFields[2], (uint8_t)payload >> 8, (uint8_t)payload};       //fix
        uint8_t receiverGeneratedMic[4];

        blake2s(receiverGeneratedMic, 4, key, keyLength, micInput, 5);

        for (int n = 0; n < 4; n++) if (receiverGeneratedMic[n] != mic[n]) return;

        devices[x].seqNum = sequenceNum;
        if (devices[x].activated == true) {
          Serial.println("\n---------- Before decryption ----------");
          Serial.print("Device address:   ");
          Serial.print(deviceAddress);
          Serial.println("\t\t\t\t|\t12 bits");
          Serial.print("Sequence number:  ");
          Serial.print(sequenceNum);
          Serial.println("\t\t\t\t|\t12 bits");
          Serial.print("Ciphertext:       ");
          Serial.print(payload);
          Serial.print("\t\t\t\t|\t");
          Serial.print(sizeof(payload));
          Serial.println(" bytes");
          Serial.print("MIC:              ");
          for (int i = 0; i < 4; i++) {
            Serial.print(mic[i]);
          }
          Serial.print("\t\t\t|\t");
          Serial.print(sizeof(mic));
          Serial.println(" bytes");
          Serial.print("Secret key:       ");
          for (int k = 0; k < 8; k++) {
            Serial.print(devices[x].secretkey[k]);
          }
          Serial.println("\t\t|");
          Serial.println("----------------------------------------");

          // Decrypt
          //Serial.println("Initiating decryption...");
          uint8_t IV[4];
          getAddressSequence(IV, deviceAddress, sequenceNum);
          uint32_t msgKey = getMsgKey(IV, key);
          uint16_t text = payload ^ (uint16_t)msgKey;

          Serial.println("---------- After decryption ----------");
          Serial.print("Device address:   ");
          Serial.print(deviceAddress);
          Serial.println("\t\t\t\t|\t12 bits");
          Serial.print("Sequence number:  ");
          Serial.print(sequenceNum);
          Serial.println("\t\t\t\t|\t12 bits");
          Serial.print("Plaintext:        ");
          Serial.print(text);
          Serial.print("\t\t\t\t|\t");
          Serial.print(sizeof(text));
          Serial.println(" bytes");
          Serial.println("--------------------------------------");

          //Serial.print("Received message: ");
          //Serial.println(text);
        } else if (devices[x].activated == false) {
          Serial.println("\n..... Secret key derivation .....");
          // Generate secret key
          /*uint8_t *hashedNonce;
            hashedNonce = blakePlaceholder(payload);
            Serial.println("inbetween");
            for (int j = 0; j < 8; j++) {
            Serial.print(hashedNonce[j]);
            }
            Serial.println();*/
          static uint8_t longNonce[8];

          uint8_t nonceInput[2] = {(uint8_t)(payload>>8), (uint8_t)payload};    
          
          blake2s(&longNonce, 8, devices[x].rootkey, 16, nonceInput, 2);

          deriveSecretKey(longNonce, x);
        } else {
          //Serial.print("WTF, should not be happening!!!\n");
        }

      } else {
      }
    }
  }
  //transmitPacket();

}

void transmitPacket () {
  LoRa.enableInvertIQ();
  LoRa.idle();
  for (int i = 0; i < 10; i++) {
    delay(50);
    Serial.println("transmitted!!!!!!!!!!!!!");
    LoRa.beginPacket();
    LoRa.print("hello");
    LoRa.endPacket();
  }
  LoRa.disableInvertIQ();
  LoRa.receive();
}
