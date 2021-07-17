/* ESP8266 AWS IoT example by Evandro Luis Copercini
   Public Domain - 2017
   But you can pay me a beer if we meet someday :D
   This example needs https://github.com/esp8266/arduino-esp8266fs-plugin

  It connects to AWS IoT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ArduinoJson.h>

// Update these with values suitable for your network.

const char* ssid = "grwb";
const char* password = "lemonslimes";

const int ZONE1 = 16;
const int ZONE2 = 15;
const int ZONE3 = 13;
const int ZONE4 = 12;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* AWS_endpoint = "a17xxrmnoajneh-ats.iot.us-west-2.amazonaws.com"; //MQTT broker ip

class Switch {
    int PIN0, PIN1;
  public:
    void init (int pin0, int pin1) {
      PIN0 = pin0;
      pinMode(PIN0, OUTPUT);
      digitalWrite(PIN0, HIGH);
      PIN1 = pin1;
      pinMode(PIN1, OUTPUT);
      digitalWrite(PIN1, HIGH);
    }
    void flip () {
      digitalWrite(PIN0, !digitalRead(PIN0));
      digitalWrite(PIN1, !digitalRead(PIN1));
    }
    void flip_off () {
      digitalWrite(PIN0, HIGH);
      digitalWrite(PIN1, HIGH);
    }
    void flip_on () {
      digitalWrite(PIN0, LOW);
      digitalWrite(PIN1, LOW);
    }
    void setto (boolean value) {
      if (!value) {
        digitalWrite(PIN0, HIGH);
        digitalWrite(PIN1, HIGH);
      } else {
        digitalWrite(PIN0, LOW);
        digitalWrite(PIN1, LOW);
      }
    }
};

Switch switch1;
Switch switch2;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<200> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  int swtch = doc["switch"];
  boolean state = doc["state"];
//  long time = doc["time"];
//  double latitude = doc["data"][0];
//  double longitude = doc["data"][1];

  Serial.print("Switch: ");
  Serial.print(swtch);
  Serial.print(" | state: ");
  Serial.print(state);
  Serial.print("\n");

  if (swtch == 1) {
    switch1.setto(state);
  } else if (swtch == 2) {
    switch2.setto(state);
  }

}
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set  MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while(!timeClient.update()){
    timeClient.forceUpdate();
  }

  espClient.setX509Time(timeClient.getEpochTime());

}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("mything")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      espClient.getLastSSLError(buf,256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  switch1.init(ZONE1, ZONE2);
  switch2.init(ZONE3, ZONE4);
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

  // Load certificate file
  File cert = SPIFFS.open("/mything.cert.der", "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");

  delay(1000);

  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");

  // Load private key file
  File private_key = SPIFFS.open("/mything.private.der", "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");



    // Load CA file
    File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
    if (!ca) {
      Serial.println("Failed to open ca ");
    }
    else
    Serial.println("Success to open ca");

    delay(1000);

    if(espClient.loadCACert(ca))
    Serial.println("ca loaded");
    else
    Serial.println("ca failed");

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
}



void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

//  long now = millis();
//  if (now - lastMsg > 2000) {
////    switch2.flip();
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 75, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("outTopic", msg);
//    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
//  }
}
