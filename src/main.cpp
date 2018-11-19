#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <Wire.h>

RCSwitch mySwitch = RCSwitch();

const char* ssid = "Aarons WLAN";
const char* password = "UIKH-FFFL-OIZN-PLOP";
const char* mqtt_server = "192.168.0.108";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

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

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg = msg + (char)payload[i];
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (msg.indexOf("true") >= 0) {
    Serial.println("true");
    mySwitch.send(23600, 24);
    Serial.println("send: 23600");
    client.publish("/all/aaron/light/chain/chain_state", "true");
  }
  else if (msg.indexOf("false") >= 0) {
    Serial.println("false");
    mySwitch.send(23500, 24);
    Serial.println("send: 23500");
    client.publish("/all/aaron/light/chain/chain_state", "false");
  }
  else if (msg.indexOf("false") == 40) {
    Serial.println("false");
    mySwitch.send(23041, 24);
    Serial.println("send: 23041");
    client.publish("/all/aaron/light/chain/chain_state", "41");
  }
  else if (msg.toInt() <= 100 || (msg.toInt() >= 0)) {
    Serial.print("set brightness to: ");
    Serial.print(msg);
    Serial. println("%");
    int value = 23000;
    value = value + msg.toInt();
    Serial.print("send: ");
    Serial.println(value);
    mySwitch.send(value, 24);
    client.publish("/all/aaron/light/chain/chain_state", payload, sizeof(msg));
  }
  else {
    Serial.println("unknown command");
    client.publish("/all/aaron/light/chain/chain_state", "unknown");
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/all/aaron/light/chain/chain_state", "started");
      // ... and resubscribe
      client.subscribe("/all/aaron/light/chain/chain_set");
      client.subscribe("/all/aaron/light/chain/chain_setbrightness");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup begin");
  mySwitch.enableTransmit(2);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("setup finished");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
