/////////////////////////////////////////////
// Description
// End device program
// Bachelor project Comtek 6 AAU - group 618 


/////////////////////////////////////////////
// Libraies to import
#include <SPI.h>
#include <LoRa.h>

//////////////////////////////////////////////
// Variables
#define BUTTON_PIN 4          // button pin number

String outgoing;              // outgoing message
byte seqNum = 0;              // sequence number of outgoing messages
byte localAddress = 0xBB;     // address of the end device
byte destination = 0xAF;      // destination of the gateway

/////////////////////////////////////////////
// Setup environment
void setup()
{
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  
  while (!Serial);
  Serial.println("LoRa Sender");
  if (!LoRa.begin(868100000)) {   // EU FQ
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

/////////////////////////////////////////////
// Main program
void loop()
{
  byte buttonState = digitalRead(BUTTON_PIN);

  // Send message when button is pressed
  if (buttonState == HIGH) {
    String message = "Test " + String(seqNum); 
    sendMessage(message);
    delay(2000);
    }
  else {
    delay(100);
  }
  delay(100);
}


/////////////////////////////////////////////
// Function
void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(seqNum);                   // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  seqNum++;                           // increment message ID

  Serial.println("Transmitter ID: " + String(localAddress));
  Serial.println("Receiever ID: " + String(destination));
  Serial.println("Sequence number: " + String(seqNum));
  Serial.println("Payload length: " + String(outgoing.length()));
  Serial.println("Payload: " + String(outgoing)+ "\n");
  delay(100);
}
