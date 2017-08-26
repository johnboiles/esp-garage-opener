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

// NOTE: These messages tend to be longer than PubSubClient likes so you need to modify
// MQTT_MAX_PACKET_SIZE in PubSubClient.h
const PROGMEM char* garage_door_config_message = "{\"name\": \"Esp8266 Temperature\", \"device_class\": \"sensor\", \"unit_of_measurement\": \"°C\"}";

// Home-assistant auto-discovery <discovery_prefix>/<component>/[<node_id>/]<object_id>/<>
const PROGMEM char* garage_door_event_topic = "homeassistant/sensor/esp8266/humidity/event";
const PROGMEM char* garage_door_state_topic = "homeassistant/sensor/esp8266/temperature/state";
const PROGMEM char* garage_door_config_topic = "homeassistant/sensor/esp8266/humidity/config";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
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

bool has_sent_discover_config = false;

void send_discover_config() {
  client.publish(garage_door_config_topic, garage_door_config_message, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DLOG("Attempting MQTT connection...\n");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      DLOG("MQTT connected\n");
      if (!has_sent_discover_config) {
        send_discover_config();
      }
      client.subscribe("garage/#");
    } else {
      DLOG("MQTT failed rc=%d try again in 5 seconds\n", client.state());
    }
  }
}

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
      pinMode(RELAY_PIN, HIGH);
      delay(600);
      pinMode(RELAY_PIN, LOW);
    }
  }
}

void checkDoorState() {
  //Checks if the door state has changed, and MQTT pub the change
  last_state = door_state; //get previous state of door
  if (digitalRead(DOOR_PIN) == 0) // get new state of door
    door_state = "OPENED";
  else if (digitalRead(DOOR_PIN) == 1)
    door_state = "CLOSED";

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
  ArduinoOTA.handle();
  yield();
  client.loop();
  Debug.handle();

  long now = millis();

  if (client.connected()) {

  } else {
    reconnect();
  }
}
