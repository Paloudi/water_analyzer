/*
 * NODE MCU
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <SimpleTimer.h>
#include <string>

using namespace std;

string data;

char ssid[] = "blabla";
char pass[] = "a76780a745d6";
char mqttServer[] = "38600.ddns.net";
int mqttPort = 1883;
char mqttUser[] = "water";
char mqttPassword[] = "mdp_de_ses_morts";

SimpleTimer timer;
WiFiClient espClient;
PubSubClient client(espClient);

char bfr[101];
string baseTemp = "temp:";
string basePH = "ph:";
string baseORP = "orp:";
string baseWater = "wt:";
int stringComplete = 0;

// Returns true if s1 is beginning of s2
int startWith(string beg, string str) { 
  int begLength = beg.length();
  int strLength = str.length();

  if (begLength > strLength) {
    return -1;
  }

  for (int i = 0; i < begLength; ++i) {
    if (beg[i] != str[i]) {
      return -1;      
    }
  }
 
  return 1;
}

string getValue(string input, char delimiter = ':') {
  size_t pos = input.find(delimiter);
  return input.substr(pos+1);
}

float getTempValue() {
  string val = getValue(bfr);
  Serial.print("sending temp at value:");
  Serial.println(val.c_str());
  return stof(val.c_str(), NULL);
}

float getPHValue() {
  string val = getValue(bfr);
  Serial.print("sending PH at value:");
  Serial.println(val.c_str());
  return stof(val.c_str(), NULL);
}

int getORPValue() {
  string val = getValue(bfr);
  Serial.print("sending ORP at value:");
  Serial.println(val.c_str());
  return stoi(val.c_str(), NULL);
}

int getWaterLevelValue() {
  string val = getValue(bfr);
  Serial.print("sending Water level at value:");
  Serial.println(val.c_str());
  return stoi(val.c_str(), NULL);
}

void uptimeEvent() {
  int uptime = millis();
  std::string s = std::to_string(uptime);
  publishSerialData("Uptime", s.c_str());
}

void phValueHandler() {
  float val = getPHValue();
  std::string s = std::to_string(val);
  publishSerialData("PH", s.c_str());
}

void tempValueHandler() {
  float val = getTempValue();
  std::string s = std::to_string(val);
  publishSerialData("Temp", s.c_str());
}

void orpValueHandler() {
  int val = getORPValue();
  std::string s = std::to_string(val);
  publishSerialData("ORP", s.c_str());
}

void waterValueHandler() {
  int val = getWaterLevelValue();
  std::string s = std::to_string(val);
  publishSerialData("water", s.c_str());
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "NodeMCUClient-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);
  
  setup_wifi();
  
  client.setServer(mqttServer, mqttPort);
  reconnect();
  
  timer.setInterval(5000L, uptimeEvent);
}

void publishSerialData(const char *ch, const char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  client.publish(ch, serialData);
}

void loop() {
   client.loop();
   timer.run(); // Initiates timer
   if (Serial.available() > 0) {
     memset(bfr,0, 101);
     Serial.readBytesUntil('\n',bfr, 100);
     if (startWith(baseTemp, bfr) > 0) {
      tempValueHandler();
     } else if (startWith(basePH, bfr) > 0) {
      phValueHandler();
     } else if (startWith(baseORP, bfr) > 0) {
      orpValueHandler();
     } else if (startWith(baseWater, bfr) > 0) {
      waterValueHandler();
     } else {
      Serial.println(bfr);
      Serial.println("was not found in any start");
     }
   }
 } 
