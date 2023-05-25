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
#define DATA_PROCESS_PIN 5    // data processing pin number
#define DATA_TRANSMIT_PIN 6   // data transmitting pin number
#define DATA_RECEIVE_PIN 7    // data receive pin number

String outgoing;              // outgoing message
byte seqNum = 0;              // sequence number of outgoing messages
byte localAddress = 0xBB;     // address of the end device
byte destination = 0xAF;      // destination of the gateway
int events = 10;              // number of packets to process and transmit 
bool done = false;

/////////////////////////////////////////////
// Setup environment
void setup()
{
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(DATA_PROCESS_PIN, OUTPUT);
  pinMode(DATA_TRANSMIT_PIN, OUTPUT);
  
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
  delay(1000);    // Let the standard power consumption stabilize
  //
  if (done == false){
    Serial.println("Program begins");
    for (int i=0; i < events; i++){
      digitalWrite(DATA_PROCESS_PIN, HIGH);     // [START] Data processing

      /// Data processing 
      String message = "Test " + String(seqNum); 
      sendMessage(message);
      ///
      
      digitalWrite(DATA_TRANSMIT_PIN, LOW);     // [STOP] Data transmission
    }
    
    delay(500);     // Delay between transmit and receive mode
      
    ////// Receive program 
    digitalWrite(DATA_RECEIVE_PIN, HIGH);       // [START] Wait for incoming data
    for (int i = 0; i<10; i++){
        onReceive(LoRa.parsePacket());
        delay(100);  
    }
    digitalWrite(DATA_RECEIVE_PIN, LOW);        // [STOP] Wait for incoming data
    /// 
  }
    done = true;
  if (done = true){
    Serial.println("Program is done");
  }
    
  }
  


/////////////////////////////////////////////
// Functions

//// Data processing and data transmit  
void sendMessage(String outgoing) {
  ////////////////////////////////////////////////////////////////////
  /* 
  - Assemble fields and payload
  - Encrypt payload
  - Create and insert MIC
  */
  LoRa.beginPacket();                       // start packet
  LoRa.write(destination);                  // add destination address
  LoRa.write(localAddress);                 // add sender address
  LoRa.write(seqNum);                       // add message ID
  LoRa.write(outgoing.length());            // add payload length
  LoRa.print(outgoing);                     // add payload
  digitalWrite(DATA_PROCESS_PIN, LOW);      // [STOP] Data processing
  
  /*
   Data processing is done
   */
  ////////////////////////////////////////////////////////////////////


  ////////////////////////////////////////////////////////////////////
  /*
   Transmit LoRaCCP header 
   */
  digitalWrite(DATA_TRANSMIT_PIN, HIGH);    // [START] Data transmission
  LoRa.endPacket();                         // finish packet and send it
  seqNum++;                                 // increment message ID
  digitalWrite(DATA_PROCESS_PIN, LOW);      // [STOP] Data transmission

  /*
   Data transmist is done
   */
  ////////////////////////////////////////////////////////////////////
  //Serial.println("Transmitter ID: " + String(localAddress));
  //Serial.println("Receiever ID: " + String(destination));
  //Serial.println("Sequence number: " + String(seqNum));
  //Serial.println("Payload length: " + String(outgoing.length()));
  //Serial.println("Payload: " + String(outgoing)+ "\n");
  //delay(100);
}

//// Receive mode
void onReceive(int packetSize) {
  ////////////////////////////////////////////////////////////////////
  /* 
  - Division of received package
  - Verify intregity
  - Decrypt payload
  - Perform request in payload
  */
  
  if (packetSize == 0) return;          // if there's no packet, return

  }
  // Handle received data package
  
  
  /*
   Receive program is done
  */
  ////////////////////////////////////////////////////////////////////
