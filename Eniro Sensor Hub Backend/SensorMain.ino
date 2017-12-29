/*--------------------------------------------------------------------*/
/*                      ENVIRO BUILDING SENSOR HUB                    */
/*--------------------------------------------------------------------*/
/*Code name - SensorCode-V2                                           */
/*Last Edited - 17.Feb.2017 by Jerome Clinton                         */
/*--------------------------------------------------------------------*/
/*Copyrights                                                          */
/*Author(s) - Mr Jerome Clinton and Mr James Barret                   */
/*COmpany - Enviro Building services PVT and NubeIOT PVT              */ 
/*--------------------------------------------------------------------*/
/*Description - This code gets CO2, temperature, humidity, ambient    */
/*light level and also detects motion and posts all the data on a     */
/*webpage. This data is later scrapped by a node-red flow to analaysis*/
/*and automation of services of a building such as HVACs, lighting and*/
/*security.                                                           */
/*--------------------------------------------------------------------*/
/*NOTE 1 - The webserver settings must be changed according to your   */
/* or dynamic network.                                                */
/*NOTE 2 - The SimpleTimer library can be found in the github repo    */
/*NOTE 3 - if the input voltage is 7 - 14V(using the freetronics      */
/*midspan injectoror any other POE switch) to operate on POE mode,    */
/*make sure to use two jumpers across the POE headers, so that the    */
/*POE"+" is connected to VIN and the POE"-" is connected to the GND   */


#define I2C_ADDRESS 0x50
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <SimpleTimer.h>          //this library is very important
#include <avr/interrupt.h>        // Library to use interrupt

//Do not change the pin numbers assigned to the variables, the code will not work otherwise.
byte mac[6];                      // MAC address byte
char rawChar;                     // temp storage for reading in from COZIR sensor UART
String inputString = "";          // a string to hold incoming data
String outputString = "";         // a string to save the COZIR sensor raw data when reading complete
boolean stringComplete = false;   // whether the string is complete
bool PIRoutput = false; 
const int ledPin = 3;             // ouput pin for the PIR status LED
int pirState = LOW;               // PIR state value(assuming initaially no motion detected)
int LastPIRstate = 0; 
const int interruptPin = 2;       // interrupt pin for PIR motion sensor output
int ldrPin = 0;                   // analog input pin for LDR light sensor

// the timer object
SimpleTimer timer;


//Change the code ip adderess fields from line 47 to 58 to configure the Server
// the dns server ip
//IPAddress dnServer(192, 168, 0, 1);
// the router's gateway address:
IPAddress gateway(192, 168, 15, 1);
// the subnet:
IPAddress subnet(255, 255, 255, 0);

// set a valid IP address
IPAddress ip(192, 168, 15, 160);

// serial ports for reading COZIR serial data
SoftwareSerial mySerial(8, 9); // RX, TX

// setup ethernet server ports
EthernetServer server(80);      // deafult page (directory and COZIR sensor data)

// read register to determine Etherten MAC address
byte readRegister(byte r)
{
  unsigned char v;
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(r);  // register to read
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADDRESS, 1); // read a byte
  while(!Wire.available()) {
    ; // wait
  }
  v = Wire.read();
  return v;
}

void recvCozirData() {
  if (mySerial.available() > 0) 
  {
    //grabs the incoming data characters one by one and appends 
    //to a string variable called inputstring
    rawChar = mySerial.read();
    inputString += rawChar;
    if (rawChar == '\n') 
    {
      stringComplete = true;
    }
  }
}

// interrupt call for change in motion
void motion(){
  
  if(digitalRead(interruptPin) ==  HIGH)
  {
    pirState = HIGH;
  }
  else
  {
    pirState = LOW;
  }
  
  

}

// a function to reset the AVR using Simple Timer
void softReset() {
  wdt_enable(WDTO_15MS); // turn on the WatchDog and don't stroke it
  for(;;) { 
    // do nothing and wait for the eventual...
  }
}

void setup() {
  MCUSR = 0;  // clear out any flags of prior resets
  timer.setTimeout(30*60*1000, softReset); // setup timer that calls reset functiin after 0.5hrs
  pinMode(ledPin, OUTPUT);        // declare LED output
  pinMode(interruptPin, INPUT);   // declre interrupt pin input
  digitalWrite(interruptPin, LOW);
  // setup interrupt for PIR motion detection
  attachInterrupt(digitalPinToInterrupt(interruptPin), motion, CHANGE);
  // Open serial communications and wait for port to open:
  mySerial.begin(9600);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(50);
  // get MAC address from the MAC chip
  Wire.begin();
  mac[0] = readRegister(0xFA);
  mac[1] = readRegister(0xFB);
  mac[2] = readRegister(0xFC);
  mac[3] = readRegister(0xFD);
  mac[4] = readRegister(0xFE);
  mac[5] = readRegister(0xFF);
  // start the Ethernet connection and the servers
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  Serial.println(" ");
}

void loop() {
  timer.run();
  digitalWrite(ledPin, pirState);
  recvCozirData();
  
  if (stringComplete == true) {
    outputString = inputString;
    Serial.println(outputString);
  }

  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client!");
    Serial.println("...");
    connectToClient(client);
    Serial.println("Client disconnected!");
    Serial.println(" ");
  }

  if (stringComplete == true) {

    inputString = "";
    stringComplete = false;
  }
  

}

// deafult
void connectToClient(EthernetClient client) {
  // an http request ends with a blank line
  boolean currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      // if you've gotten to the end of the line (received a newline
      // character) and the line is blank, the http request has ended,
      // so you can send a reply
      if (c == '\n' && currentLineIsBlank) {
        // send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connnection: close");
        client.println();
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        // add a meta refresh tag, so the browser pulls again every 10 seconds:
        client.println("<meta http-equiv=\"refresh\" content=\"5\">");
        client.println("<h1>Enviro Sensor Hub WebServer</h1>");
        client.println("<br />");
        client.println("<h2>COZIR Raw Data:</h2>");
        client.print("<p1>");
        client.print(outputString);
        client.print("</p1>");
        client.println("<h2>PIR Sensor Status:</h2>");
        client.print("<p2>");
        if (pirState == HIGH) {
          client.print("HIGH");
        }
        else
          client.print("LOW");
        client.print("</p2>");
        client.println("<h2>LDR Raw Analog Value:</h2>");
        client.print("<p3>");
        int raw = analogRead(ldrPin);   
        client.print(raw);
        client.println("</p3>");
        client.println("<br />");
        client.println("</html>");
        break;
      }
      if (c == '\n') {
        // you're starting a new line
        currentLineIsBlank = true;
      } 
      else if (c != '\r') {
        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }
  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client.stop();
}
