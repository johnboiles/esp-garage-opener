#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "secrets.h"

#if LOGGING
#include <RemoteDebug.h>
#define DLOG(msg, ...) if(Debug.isActive(Debug.DEBUG)){Debug.printf(msg, ##__VA_ARGS__);}
RemoteDebug Debug;
#else
#define DLOG(msg, ...)
#endif

#define HOSTNAME "garage-door"

#define mqtt_version MQTT_VERSION_3_1_1
#define mqtt_server "10.0.0.2"
#define mqtt_user "homeassistant"

//Define the pins
#define RELAY_PIN D2
#define DOOR_PIN D1

const char compile_date[] = __DATE__ " " __TIME__;

//Setup Variables
String switch1;
String strTopic;
String strPayload;
char* door_state = (char *)"UNDEFINED";
char* last_state = (char *)"";
long lastMsg = 0;

const PROGMEM char* button_topic = "garage/button";
const PROGMEM char* door_topic = "garage/door";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void callback(char* topic, byte* payload, unsigned int length) {
  //if the 'garage/button' topic has a payload "OPEN", then 'click' the relay
  payload[length] = '\0';
  strTopic = String((char*)topic);
  if (strTopic == button_topic)
  {
    switch1 = String((char*)payload);
    if (switch1 == "OPEN")
    {
      //'click' the relay
      Serial.println("ON");
      digitalWrite(RELAY_PIN, HIGH);
      delay(600);
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(DOOR_PIN, INPUT_PULLUP);

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Built on");
  Serial.println(compile_date);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Set Hostname.
  String hostname(HOSTNAME);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();

  #if LOGGING
  Debug.begin((const char *)hostname.c_str());
  Debug.setResetCmdEnabled(true);
  #endif
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DLOG("Attempting MQTT connection...\n");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      DLOG("MQTT connected\n");
      client.subscribe("garage/#");
    } else {
      DLOG("MQTT failed rc=%d try again in 5 seconds\n", client.state());
    }
  }
}

void checkDoorState() {
  //Checks if the door state has changed, and MQTT pub the change
  last_state = door_state; //get previous state of door
  if (digitalRead(DOOR_PIN) == 1) // get new state of door
    door_state = (char *)"OPENED";
  else if (digitalRead(DOOR_PIN) == 0)
    door_state = (char *)"CLOSED";

  if (last_state != door_state) { // if the state has changed then publish the change
    client.publish(door_topic, door_state);
    Serial.println(door_state);
  }
  //pub every minute, regardless of a change.
  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    client.publish(door_topic, door_state);
  }
}

void loop() {
  //If MQTT client can't connect to broker, then reconnect
  if (!client.connected()) {
    reconnect();
  }
  checkDoorState();
  ArduinoOTA.handle();
  yield();
  client.loop();
  Debug.handle();
}
