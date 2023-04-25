// Libraies to import
#include <SPI.h>
#include <LoRa.h>

uint16_t id = 0x61A8; 
uint16_t XOR = 0xFFFF;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  uint16_t new_id;
  new_id = LoRa.deviceID(id);
  
  Serial.println("Dev_ID: " + String(id));
  Serial.println("New_ID: " + String(new_id));
  Serial.println("Back_ID: " + String(new_id xor XOR) + "\n");
   
  
}
