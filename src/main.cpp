#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "secrets.h"

// Debug logging over telnet. Just run:
// telnet garage-door.local
#if LOGGING
#include <RemoteDebug.h>
#define DLOG(msg, ...) if(Debug.isActive(Debug.DEBUG)){Debug.printf(msg, ##__VA_ARGS__);}
RemoteDebug Debug;
#else
#define DLOG(msg, ...)
#endif

// Network setup
#define HOSTNAME "garage-door"
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// MQTT Setup
#define mqttServer "10.0.0.2"
#define mqttUser "homeassistant"
const PROGMEM char *eventTopic = "garage/button";
const PROGMEM char *stateTopic = "garage/door";

// Define the pins
#define RELAY_PIN D2
#define DOOR_PIN D1

const char compile_date[] = __DATE__ " " __TIME__;

typedef enum {
  DoorStateUndefined = 0,
  DoorStateOpened,
  DoorStateClosed
} DoorState;
DoorState doorState = DoorStateUndefined;
long lastStateMsgTime = 0;

const char *doorStateString(DoorState doorState) {
  switch(doorState) {
    case DoorStateOpened:
      return "OPENED";
    case DoorStateClosed:
      return "CLOSED";
    default:
      return "UNDEFINED";
  }
}

// Compares expected string and payload bytes to see if they match. Useful since
// payload isn't null terminated
bool comparePayload(const char *expected, byte *payload, unsigned int length) {
  // If lengths are different they are definitely different
  if (strlen(expected) != length) {
    return false;
  }
  // Compare the contents
  return memcmp((const void *)expected, (const void *)payload, length) == 0;
}

void callback(char *topic, byte *payload, unsigned int length) {
  DLOG("Received callback for topic %s\n", topic);
  // If the 'garage/button' topic has a payload "OPEN", then trigger the relay
  if (strcmp(eventTopic, topic) == 0) {
    if (comparePayload("OPEN", payload, length)) {
      // Trigger the relay for 1s
      DLOG("Clicking the relay\n");
      digitalWrite(RELAY_PIN, HIGH);
      delay(1000);
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(DOOR_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("Built on");
  Serial.println(compile_date);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Set Hostname.
  String hostname(HOSTNAME);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();

  #if LOGGING
  Debug.begin((const char *)hostname.c_str());
  Debug.setResetCmdEnabled(true);
  #endif
}

void reconnect() {
  DLOG("Attempting MQTT connection...\n");
  // Attempt to connect
  if (client.connect(HOSTNAME, mqttUser, mqttPassword)) {
    DLOG("MQTT connected\n");
    client.subscribe(eventTopic);
  } else {
    DLOG("MQTT failed rc=%d try again in 5 seconds\n", client.state());
  }
}

// Checks if the door state has changed, and MQTT pub the change
void checkDoorState() {
  // Get previous state of door
  DoorState lastState = doorState;
  // Get new state of door
  if (digitalRead(DOOR_PIN) == 1) {
    doorState = DoorStateOpened;
  } else if (digitalRead(DOOR_PIN) == 0) {
    doorState = DoorStateClosed;
  }

  // If the state has changed then publish the change
  if (lastState != doorState) {
    DLOG("Door state changed to %s (%d), publishing\n", doorStateString(doorState), doorState);
    client.publish(stateTopic, doorStateString(doorState));
  }

  // Publish every minute, regardless of a change.
  long now = millis();
  if (now - lastStateMsgTime > 60000) {
    lastStateMsgTime = now;
    DLOG("Performing periodic publish for state %s (%d)\n", doorStateString(doorState), doorState);
    client.publish(stateTopic, doorStateString(doorState));
  }
}

void loop() {
  // If MQTT client can't connect to broker, then reconnect
  if (!client.connected()) {
    reconnect();
  } else {
    checkDoorState();
  }
  ArduinoOTA.handle();
  yield();
  client.loop();
  Debug.handle();
}
