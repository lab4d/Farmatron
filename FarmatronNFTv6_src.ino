// ****  HYDROPONIC COMPUTER SOFTWARE v6  ****
// ****          Lab4D 2020              ****

//fix mqqt sometimes sending crazy numbers

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include "DHTesp.h"

#define R1 16 // D0
#define R2 5  // D1
#define P1 4  // D2
#define DHTPin 2 // D4

// Farmatron Variable Variables™
long uploadLoop = 420000; //Thingspeak send interval in millisec - 15000 minimum (default: 420000)
unsigned long onInterval = 30000;
unsigned long offInterval = 300000;

bool UseServer = true;
bool UseThingSpeak = true;
bool UseSerial = false;
bool UseMQTT = true;

// WiFi Credentials
const char* ssid = "WIFI SSID";
const char* password = "WIFI PASSWORD";
// Static IP address configuration 
IPAddress staticIP(192, 168, 1, 31); 
IPAddress gateway(192, 168, 1, 1);   
IPAddress subnet(255, 255, 255, 0);  
IPAddress dns(8, 8, 8, 8);  
const char* deviceName = "Farmatron";

// MQTT Settings
const char* mqtt_server = "192.168.1.43";
const char* mqtt_user = "MQTT USERNAME";
const char* mqtt_password = "MQTT PASSWORD";
const char* clientID = "PlantInoMQTT";
const char* outTopic = "active";
const char* outWTopic = "plantino/plantWaterState";
const char* outFTopic = "plantino/plantFanState";
const char* outTTopic = "plantino/tempState";
const char* outHTopic = "plantino/humState";
const char* inTopic = "plantino/toggleFan";

//ThingSpeak Connection Settings
const char* server = "api.thingspeak.com";
String apiKey = "THINGSPEAK KEY";
String Channel = "CHANNEL NAME";

// Farmatron Internal Variables
float temperature;
float Ctemperature;
float humidity;
float daylight;
bool pumpState;
bool offwait;
bool fanState = false;
int Rcounter;
unsigned long previousMillis;
unsigned long before;
unsigned long now;
unsigned long cycle;
unsigned long sent = 1;
int onT = onInterval/1000;
int offT = offInterval/1000;
const char* imagefile = "/image.png";
const char* htmlfile = "/index.html";

// Initialize Libraries
MDNSResponder mdns;
ESP8266WebServer srv(80);
WiFiClient client;
PubSubClient MQTTclient(client);
DHTesp dht;

// SETUP
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Farmatron v2 starting up...");
  Serial.print("Water for " +String(onT));
  Serial.print(" seconds every ");
  Serial.print(String(offT) + " seconds.");
  Serial.println();

  dht.setup(DHTPin, DHTesp::DHT22);
  
  connectWiFi();
  
  if (UseServer){
    SPIFFS.begin();
    // Setup HTTP Server handling
    srv.on("/", handleRoot);
    srv.onNotFound(handleWebRequests);      
    srv.on("/setActive", handleSActive);
    srv.on("/setPassive", handleSPassive);
    srv.on("/setUpload", handleSUpload);
    srv.on("/setFan", handleSFan);
    srv.on("/getFan", handleGFan);
    srv.on("/getUpload", handleGUpload);
    srv.on("/getPump", handleGPump);
    srv.on("/getActive", handleGActive);
    srv.on("/getPassive", handleGPassive);
    srv.on("/getTemp", handleGTemp);
    srv.on("/getHum", handleGHum);
    srv.begin();
    Serial.println("HTTP Server Started");
  }
 
  Serial.println();
  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(P1, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(R1, HIGH);
  digitalWrite(R2, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  if (UseMQTT){
    MQTTclient.setServer(mqtt_server, 1883);
    MQTTclient.setCallback(callback);
  }
  
  toggleFan();
  
  delay(1000);
  before=offInterval + 4200;  //force launch on first loop cycle
}

// Main Loop
void loop() {
  getSensors();
  if (UseServer){
    srv.handleClient();
  }
  
  now = millis();
  if (now - before > offInterval) {  // Pump Active 
    before = now;
    digitalWrite(P1, HIGH);            // Turn On Circulation Pump 
    //digitalWrite(R2, LOW);            // Turn On Pump
    digitalWrite(LED_BUILTIN, LOW);
    pumpState = true;
    offwait = true;
    cycle++;
    if (!UseSerial){
      Serial.println("Watering - " + String(onT));
    }
    if (UseThingSpeak){
      Serial.println ("UPLOAD");
      sendThing();
    }
    if(UseMQTT){
      reconnect();
      doMQTT();
    delay(100);
  } 
  }
  if (now > before + onInterval){  // Pump Inactive 
    if (offwait){
      digitalWrite(P1, LOW);         // Turn Off Circulation Pump
      //digitalWrite(R2, HIGH);         // Turn Off Pump
      digitalWrite(LED_BUILTIN, HIGH);
      pumpState = false;
      offwait=false;
      if (!UseSerial){
        Serial.println("Waiting - " + String(offT));
      }
      if(UseMQTT){
        doMQTT();
      delay(100);
      }
   }
  }  
         
  if(UseSerial){
    printSerial();
    delay(100);
  }  
                  
}

// Process MQTT
void doMQTT (){
  // Prepare and Send data over MQTT
  if (!MQTTclient.connected()) {
        //reconnect();
      }
  MQTTclient.loop();
    char isWater[4];
    itoa (pumpState, isWater, 10);
    char isFan[4];
    itoa (fanState, isFan, 10);
    char tmp[4];
    itoa (temperature, tmp, 10);
    char hum[4];
    itoa (humidity, hum, 10);
    if (MQTTclient.connect(clientID, mqtt_user, mqtt_password)) {
      MQTTclient.publish(outWTopic, isWater);
      MQTTclient.publish(outFTopic, isFan);
      MQTTclient.publish(outTTopic, tmp);
      MQTTclient.publish(outHTopic, hum);
      MQTTclient.subscribe(inTopic);
    }
    Serial.println("MQTT Published! ");
}

// Message received over MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Activate Pump for a short time
  toggleFan();
}

// MQTT Reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (MQTTclient.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println(" connected!");
      // Once connected, publish an announcement...
      MQTTclient.publish(outTopic, "Plant Sensor Active");
      // ... and resubscribe
      MQTTclient.subscribe(inTopic);
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      if (Rcounter >= 1) {
        Rcounter = 0;
        break;
      }
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      Rcounter++;
    }
      
      
  }
}


void printSerial(){
  Serial.print("DHT");
  Serial.print("\t");
  Serial.print("HUM");
  Serial.print("\t");
  Serial.print("TEMP");
  Serial.print("\t");
  Serial.print("FEELS");
  Serial.print("\t");
  Serial.print("PUMP ON");
  Serial.print("\t");
  Serial.print("PUMP OFF");
  Serial.print("\t");
  Serial.print("WATERING");
  Serial.println();
  
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t");
  Serial.print(temperature, 1);
  Serial.print("\t");
  Ctemperature = dht.computeHeatIndex(temperature, humidity, false);
  Serial.print(Ctemperature, 1);
  Serial.print("\t");
  Serial.print(onT, 1);
  Serial.print("\t");
  Serial.print(offT, 1);
  Serial.print("\t");
  Serial.print(pumpState, 1);
  Serial.println();
}

// Read DHT Humidity and Temperature, and LDR Light
void getSensors(){
  temperature = dht.getTemperature();
  humidity = dht.getHumidity();
  Ctemperature = dht.computeHeatIndex(temperature, humidity, false);
}

void activatePump() {
  Serial.print("Watering...");
  digitalWrite(R1, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  pumpState = true;
  now = millis();
  if (now - before > onInterval) {
    before = now;
    delay(onInterval);    
    digitalWrite(R1, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    cycle++;
    Serial.println(" Done! - " + String(cycle));
    pumpState = false; 
  }
}

void toggleFan() {
  if (fanState){
    digitalWrite(R1, LOW);
    fanState=false;
    Serial.println("Fan Off");
  }
  else {
    digitalWrite(R1, HIGH);
    fanState=true;
    Serial.println("Fan On");
  }
}

void connectWiFi (){
//  WiFi.disconnect();
//  delay(10);
  Serial.println();
  Serial.println("Connecting to " + *ssid);
  WiFi.hostname("PlantIno v3");
  WiFi.config(staticIP, subnet, gateway, dns);
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA); // Make sure wifi is in the right mode
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println();
  Serial.print("WiFi connected! ");
  Serial.print("IP: ");
  Serial.println (WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP()))
    Serial.println("MDNS responder started");
}

/// Send sensor readings to ThingSpeak
void sendThing() {
  Serial.println("+++DATA UPLOAD: ON - " + String(sent));
  
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(humidity);
    postStr += "&field2=";
    postStr += String(temperature);
    postStr += "&field3=";
    postStr += String(daylight);
    postStr += "&field4=";
    postStr += String(onInterval);
    postStr += "&field5=";
    postStr += String(offInterval);
    postStr += "&field6=";
    postStr += String(fanState);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    sent++;
  }
  else {
    Serial.println("No connection - Data Not Sent");
    WiFi.disconnect(true);
    Serial.println("Connection Lost - Resetting Wifi");
    connectWiFi();
    delay(500);
  }
  client.stop();
}


void handleRoot() {
  srv.sendHeader("Location", "/index.html",true);   //Redirect to html web page
  srv.send(302, "text/plane","");
}
/// Web Request Handling
void handleWebRequests(){
  if(loadFromSpiffs(srv.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += srv.uri();
  message += "\nMethod: ";
  message += (srv.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += srv.args();
  message += "\n";
  for (uint8_t i=0; i<srv.args(); i++){
    message += " NAME:"+srv.argName(i) + "\n VALUE:" + srv.arg(i) + "\n";
  }
  srv.send(404, "text/plain", message);
  Serial.println(message);
}

bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.html";
  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (srv.hasArg("download")) dataType = "application/octet-stream";
  if (srv.streamFile(dataFile, dataType) != dataFile.size()) {
  }

  dataFile.close();
  return true;
}

void handleGPump() {
  String a = String(pumpState);
  srv.send(200, "text/plane", a); //Send to web page
}
void handleGFan() {
  String f = String(fanState);
  srv.send(200, "text/plane", f); //Send to web page
}

void handleGUpload() {
  String u = String(UseThingSpeak);
  srv.send(200, "text/plane", u); //Send to web page
}
void handleGActive() {
  String a = String(onInterval);
  srv.send(200, "text/plane", a); //Send to web page
}
void handleGPassive() {
  String p = String(offInterval);
  srv.send(200, "text/plane", p); //Send to web page
}
void handleGTemp() {
  String t = String(temperature);
  srv.send(200, "text/plane", t); //Send to web page
}
void handleGHum() {
  String h = String(humidity);
  srv.send(200, "text/plane", h); //Send to web page
}


/// Activate Fan
void handleSFan() {
 String t_state = srv.arg("fan"); 
 //fanState = !fanState;
 toggleFan();
 Serial.println("=F= " + String(fanState));
 srv.send(200, "text/plane", t_state); //Send to web page
 
}

/// Set Active Interval
void handleSActive() {
 String t_state = srv.arg("active"); 
 Serial.println("=A= " + t_state);
 onInterval = t_state.toInt();
 onT = onInterval/1000;
 srv.send(200, "text/plane", t_state); //Send to web page
}
/// Set Passive Interval
void handleSPassive() {
 String t_state = srv.arg("passive"); 
 Serial.println("=P= " + t_state);
 offInterval = t_state.toInt();
 offT = offInterval/1000;
 srv.send(200, "text/plane", t_state); //Send to web page
}

/// Upload Handling
void handleSUpload() {
 String uploadState = "OFF";
 String t_state = srv.arg("upload"); 
 if(t_state == "1")
 {
  uploadState = "ON"; //Feedback parameter
  UseThingSpeak= true;
  Serial.print("=U+=");
 }
 else
 {
  uploadState = "OFF"; //Feedback parameter 
  UseThingSpeak = false;
  Serial.print("=U-="); 
 }
 srv.send(200, "text/plane", uploadState); //Send to web page
}
