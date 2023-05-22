#define ROTL16(word, offset) (((word) << (offset)) | (word >> (16 - (offset))))
#define ROTR16(word, offset) (((word) >> (offset)) | ((word) << (16 - (offset))))
#define ROTL32(word, offset) (((word) << (offset)) | (word >> (32 - (offset))))
#define ROTR32(word, offset) (((word) >> (offset)) | ((word) << (32 - (offset))))
#define ROTL64(word, offset) (((word) << (offset)) | (word >> (64 - (offset))))
#define ROTR64(word, offset) (((word) >> (offset)) | ((word) << (64 - (offset))))

#define EncryptRound16(word1, word2, roundkey) (word1 = (ROTR16(word1, 8) + word2) ^ (roundkey), word2 = ROTL16(word2, 3) ^ word1)
#define EncryptRound32(word1, word2, roundkey) (word1 = (ROTR32(word1, 8) + word2) ^ (roundkey), word2 = ROTL32(word2, 3) ^ word1)
#define EncryptRound64(word1, word2, roundkey) (word1 = (ROTR64(word1, 8) + word2) ^ (roundkey), word2 = ROTL64(word2, 3) ^ word1)

static uint8_t k[16] = {0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x12, 0x13, 0x18, 0x19, 0x1a, 0x1b};
static uint8_t pt[8] = {0x2d, 0x43, 0x75, 0x74, 0x74, 0x65, 0x72, 0x3b};


///////////////////////////////////////////////
// Word/byte conversion

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


///////////////////////////////////////////////
// Key Schedule

void Expand64(uint32_t K[], uint32_t roundkey[]) {
  static uint32_t i, D = K[3], C = K[2], B = K[1], A = K[0];
  for (i = 0; i < 26;) {
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
    Serial.print("Pt_");
    Serial.print(i);
    Serial.print("=(");
    Serial.print(ciphertext[1], HEX);
    Serial.print(",");
    Serial.print(ciphertext[0], HEX);
    Serial.println(")"); 
  }
}




void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();
  // Test SPECK 64/128

  static uint8_t pt[8] = {0x2d, 0x43, 0x75, 0x74, 0x74, 0x65, 0x72, 0x3b};
  static uint8_t k[16]  = {0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1a, 0x1b};
  static uint32_t Pt[2];
  static uint8_t ct[8];
  static uint32_t Ct[2];

  static uint32_t K[4];
  static uint32_t rk[27];
  
  Serial.print("pt = ");
  for (int i = 0; i < 8; i++) {
    Serial.print(pt[i],HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("k = ");
  for (int i = 0; i < 16; i++) {
    Serial.print(k[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
  
  Serial.println("Running BytesToWords32(pt,Pt,8)");
  ConvertBW32(pt, Pt, 8);

  Serial.print("(Pt[1],Pt[0])=(");
  Serial.print(Pt[1], HEX);
  Serial.print(",");
  Serial.print(Pt[0],HEX);
  Serial.println(")");
  Serial.println();

  Serial.println("Running BytesToWords32(k,K,16)");
  ConvertBW32(k, K, 16);

  Serial.print("(K[3],K[2],K[1],K[0])=(");
  Serial.print(K[3], HEX);
  Serial.print(",");
  Serial.print(K[2],HEX);
  Serial.print(",");
  Serial.print(K[1], HEX);
  Serial.print(",");
  Serial.print(K[0],HEX);
  Serial.println(")");
  Serial.println();
  
  Serial.println("Running Speck64128KeySchedule(K,rk)");
  Serial.println();
  
  Expand64(K, rk);

  for (int i = 0; i < 27; i ++) {
    Serial.print("rk[");
    Serial.print(i);
    Serial.print("]=");;
    Serial.println(rk[i], HEX);
  }
  Serial.println();

  Serial.println("Running Speck64128Encrypt(Pt,Ct,rk)");
  Serial.println();
  Block64Encrypt(Pt, Ct, rk);
  Serial.println();

  Serial.print("(Ct[1],Ct[0])=(");
  Serial.print(Ct[1], HEX);
  Serial.print(",");
  Serial.print(Ct[0],HEX);
  Serial.println(")");
  Serial.println();

  Serial.println("Running Words32ToBytes(Ct,ct,2)");
  ConvertW32B(Ct, ct, 2);

  Serial.print("ct = ");
  for (int i = 0; i < 8; i++) {
    Serial.print(ct[i], HEX); 
    Serial.print(" ");
  }
  Serial.println();
  

}

void loop() {
  // Do Nothing
}
