void setup() {
  // put your setup code here, to run once:

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                     IMPORTANT THING TO REMEMBER
//
//
//                  - Set fixed length of message and key to save operations
//                  - Make another version that fits the ciphertext version
//                          - Padding to the key
//                          - 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Imports
#include <stdint.h>
#include <stddef.h>

// declarations

#ifndef ROTR32
#define ROTR32(x, y)  (((x) >> (y)) ^ ((x) << (32 - (y))))
#endif

    // #define ConvertPoint(p)    
    //     uint32_t p[0] = p[0];
    //     uint32_t p[1] = p[1];
    // (((uint32_t)((uint8_t*)(p))[0]) ^        
    // ((uint32_t)((uint8_t*)(p))[1] << 8) ^   
    // ((uint32_t)((uint8_t*)(p))[2] << 16) ^  
    // ((uint32_t)((uint8_t*)(p))[3] << 24))   

#define Mixer(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = ROTR32(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR32(v[b] ^ v[c], 12); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = ROTR32(v[d] ^ v[a], 8);  \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR32(v[b] ^ v[c], 7); }


typedef struct BlakeInput{
    uint8_t block[64];
    uint32_t hState[8];
    uint32_t total[2];
    size_t ptrB;                        // A "pointer" that is used to hold values og b
    int inlen = 2;                      // States the length of the input
    int outlen = 8;                     // Controls the length of the output
    int keyLength = 16;                  // Controls the length of the key
    uint8_t Secretkey[16];               // Variable for holding the Secret Key
};

// IV - Initialisation vectors

static const uint32_t IV[8] = {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};



////////////////////////////////////////////////
//                Functions
////////////////////////////////////////////////


// uint32_t ConvertPointer(uint8_t p[]){
//     uint32_t result[16];
//     uint32_t result[0] = p[0];
//     uint32_t result[1] = p[1];
//     uint32_t result[2] = p[2];
//     uint32_t result[3] = p[3];
// }

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

//////////////////////////////////// COMPRESSION ////////////////////////////////////////


void compress(struct BlakeInput *ptr, bool isLast){

    // SIGMA is a constant between implementations and is always exactly as it is here.
    // It is used for
    //          -
    //          - 
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

    // Create local values used for compressing
    int i;
    uint32_t v[16], m[16];

    // Instert the state vectors and initialization vectors into v[]. 
    for (i = 0; i<8; i++){
        v[i] = ptr->hState[i];
        v[i+8] = IV[i];
    }

    // Xor lowest and highest into v[]
    v[12] ^= 8;
    v[13] ^= 16;

    if (isLast){
        v[14] = ~v[14];

        ConvertBW32(ptr->block,m,64);
        }


    for (i = 0; i<16; i++){
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(1,5,9,13,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(2,6,10,14,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(3,7,11,15,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,5,10,15,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(1,6,11,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(2,7,8,13,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(3,4,9,14,m[sigma[i][0]], m[sigma[i][1]])
    }

    for (i=0; i<8; i++)
        ptr->hState[i] ^= v[i] ^v[i+8];
}

void updateStruct(struct BlakeInput *ptr, const void *input){
    int i;

    for (i=0; i<ptr->inlen; i++){
        if (ptr->ptrB == 64){
            ptr->total[0] += ptr->ptrB;
            if (ptr->total[0] < ptr->ptrB){
                ptr->total[1]++;
            }
            compress(ptr, 0);
            ptr->ptrB = 0;
        }
        ptr->block[ptr->ptrB++] = ((const uint8_t *) input)[i];
    }
}

static void setupMsg(struct BlakeInput *ptr){

    int i = 0;

    for (i=0; i<8; i++)
        ptr->hState[i] = IV[i];

    ptr->hState[0] ^= 0x01011608;
    ptr->total[0] = 0;
    ptr->total[1] = 0;
    ptr->ptrB = 0;

    for (i=ptr->keyLength; i<64; i++){
        ptr->block[i] = 0;
    }
    if (ptr->keyLength > 0) {
        updateStruct(ptr, ptr->Secretkey);
        ptr->ptrB = 64;
    }        
}

void lastRound(struct BlakeInput *ptr, void *out){

    int i;

    ptr->total[0] += ptr->ptrB;
    if (ptr->total[0] < ptr->ptrB)
        ptr->total[1]++;

    while (ptr->ptrB < 64)
        ptr->block[ptr->ptrB++] = 0;
    compress(ptr, 1);

    for (i=0; i < ptr->outlen; i++){
        ((uint8_t *) out)[i] = (ptr->hState[i>>2]>>(8*(i&3))) & 0xFF;
    }
}

//////////////////////////////////// Blaking ////////////////////////////////////////

uint16_t blake2s(uint16_t inputNonce, uint8_t rootkey[]){

    // Initialises state vectors

    
    uint16_t outputNonce;
    
    
    BlakeInput *msgInfo;
    for (int i = 0; i<16; i++){
        msgInfo->Secretkey[i] = rootkey[i];
    }

    setupMsg(msgInfo);
    updateStruct(msgInfo, &inputNonce);
    lastRound(msgInfo, &outputNonce);

    printf(outputNonce);
    


    return outputNonce;
}

////////////////////////////////////////////////
//                Main Program
////////////////////////////////////////////////

void loop() {
  // put your main code here, to run repeatedly
  printf("My spoon is too big");
}
