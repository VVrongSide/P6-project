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
#include <iostream>

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


typedef struct {
    uint8_t block[64];
    uint32_t hState[8];
    uint32_t total[2];
    size_t ptrB;                        // A "pointer" that is used to hold values og b
    int inlen = 5;                      // States the length of the input
    int outlen = 4;                     // Controls the length of the output
    int keyLength = 8;                  // Controls the length of the key
    uint8_t Secretkey[8];               // Variable for holding the Secret Key
}   BlakeInput;

// IV - Initialisation vectors

static const uint32_t IV[8] = {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};



////////////////////////////////////////////////
//                Functions
////////////////////////////////////////////////


uint32_t ConvertPointer(uint8_t p[]){
    uint32_t result[16];
    uint32_t result[0] = p[0];
    uint32_t result[1] = p[1];
    uint32_t result[2] = p[2];
    uint32_t result[3] = p[3];
}

//////////////////////////////////// COMPRESSION ////////////////////////////////////////


static void compress(BlakeInput *ptr, bool isLast){

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
    v[12] ^= 5;
    v[13] ^= 8;

    if (isLast){
        v[14] = ~v[14];

        for (i = 0; i < 16; i++){
            m[i] = ConvertPointer(ptr->block[2*i]);
        }
    }

    for (i = 0; i<16; i++){
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
        Mixer(0,4,8,12,m[sigma[i][0]], m[sigma[i][1]])
    }

    for (i=0; i<8; i++)
        ptr->hState[i] ^= v[i] ^v[i+8];
}

void updateStruct(BlakeInput *ptr, void const *in){
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
        ptr->block[ptr->ptrB++] = ((const uint8_t *) in)[i];
    }
}

static void setupMsg(BlakeInput *ptr){

    int i = 0;

    for (i=0; i<8; i++)
        ptr->hState[i] = IV[i];

    ptr->hState[0] ^= 0x01010804;
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

void lastRound(BlakeInput *ptr, void *out){

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

void blake2s(){

    // Initialises state vectors
    

    // Mix first state vector with key length (in bytes) kk and desired output length (in bytes) nn - 0x0101kknn
    // int H0 = H0 ^ 0x01010805;

    // // Init local variables used to check how far in the process we are
    // int bytesCompressed = 0;
    // int bytesremaining = msgLength;

    // // Pads The Key so that we have 128 bytes
    
    // char newMessage[] = message.append(secretKey.append(padding));
    // bytesremaining = bytesremaining + 128;

    // char block[] = newMessage.substr(0,128);
    // bytesCompressed = bytesCompressed + 128;
    // bytesremaining = bytesremaining - 128;

    uint16_t output;
    const void *InputMsg = "sometext";
    BlakeInput *msgInfo;

    setupMsg(msgInfo);
    updateStruct(msgInfo, InputMsg);
    lastRound(msgInfo, &output);

    std::cout << output;
    



}











////////////////////////////////////////////////
//                Main Program
////////////////////////////////////////////////








