/////////////////////////////////////////////
// Description
// Gateway program 
// Bachelor project Comtek 6 AAU - group 618

/////////////////////////////////////////////
// Libraies to import
#include <SPI.h>
#include <LoRa.h>

//////////////////////////////////////////////
// Variables

byte seqNum = 0;              // sequence number of outgoing messages
byte localAddress = 0xAF;     // address of the gateway
byte destination = 0xBB;      // destination of the end device

/////////////////////////////////////////////
// Setup environment
void setup()
{
  Serial.begin(9600);
  
  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(868100000)) {   // EU FQ
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

/////////////////////////////////////////////
// Main program
void loop()
{
  onReceive(LoRa.parsePacket());
  delay(100);
}


/////////////////////////////////////////////
// Function

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int receiver_address = LoRa.read();          // address of receiver device
  byte transmitter_address = LoRa.read();      // address of transmitter device
  byte seqNum = LoRa.read();                   // sequence number of message
  byte payload_length = LoRa.read();           // length of payload

  String payload = "";

  while (LoRa.available()) {
    payload += (char)LoRa.read();
    //Serial.println(payload);
  }

  // Check whether the 'payload' length matches the informed 'payload_length'  
  if (payload_length != payload.length()) {       
    Serial.println("[ERROR]: Payload length does not match length");
    return;                             
  }

  // If the transmitter isn't this device or broadcast,
  if (receiver_address != localAddress && receiver_address != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from device: " + String(transmitter_address));
  Serial.println("Sequence number: " + String(seqNum));
  Serial.println("Payload length: " + String(payload_length));
  Serial.println("Payload: " + String(payload));
  Serial.println("RSSI: " + String(LoRa.packetRssi()));		// RSSI from received package
  Serial.println("Snr: " + String(LoRa.packetSnr()) + "\n");	// Signal-to-Noise from received package
}
