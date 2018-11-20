#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <Wire.h>

RCSwitch mySwitch = RCSwitch();
RCSwitch myReceiver = RCSwitch();

//WiFi nad MQTT
const char* ssid = "Aarons WLAN";
const char* password = "UIKH-FFFL-OIZN-PLOP";
const char* mqtt_server = "192.168.0.108";

//PINs
int receiver = 4;   //D2

//receive variables
const int pir_sensor_1_motion = 13100;
int prev_signal = 0;
unsigned long timestamp = 0;

/* MQTT subscriptions
- /all/aaron/light/chain/chain_set
- /all/aaron/light/chain/chain_setbrightness
 */

/* MQTT pushs
- /all/aaron/light/chain/chain_state
- /all/aaron/motion_detect/pir1_detected
 */

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
  String message = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message = message + (char)payload[i];
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (message.indexOf("true") >= 0) {
    Serial.println("true");
    mySwitch.send(23600, 24);
    Serial.println("send: 23600");
    client.publish("/all/aaron/light/chain/chain_state", "true");
  }
  else if (message.indexOf("false") >= 0) {
    Serial.println("false");
    mySwitch.send(23500, 24);
    Serial.println("send: 23500");
    client.publish("/all/aaron/light/chain/chain_state", "false");
  }
  else if (message.toInt() == 40) {
    Serial.println("got 40 --> set 41");
    mySwitch.send(23041, 24);
    Serial.println("send: 23041");
    client.publish("/all/aaron/light/chain/chain_state", "41");
  }
  else if (message.toInt() <= 100 || (message.toInt() >= 0)) {
    Serial.print("set brightness to: ");
    Serial.print(message);
    Serial. println("%");
    int value = 23000;
    value = value + message.toInt();
    Serial.print("send: ");
    Serial.println(value);
    mySwitch.send(value, 24);
    client.publish("/all/aaron/light/chain/chain_state", payload, sizeof(value - 23000));
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
      client.publish("/all/aaron/motionDetector/pirBed/pirBed_detected", "started");
      client.publish("/all/aaron/light/chain/chain_state", "started");
      //client.publish("/all/aaron/motion_detector/pir1/pir1_detected", "started");
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
  myReceiver.enableReceive(receiver);
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



  if (myReceiver.available()) {


    int signal = myReceiver.getReceivedValue();

    Serial.println("");
    Serial.print("message received: ");
    Serial.println(signal);

    if ((!(signal == prev_signal) || millis() - timestamp > 2000) && (signal > 0) && !(signal == 23040)){  //23040 is a frequently send code (seems to be an error - needs to be catched)
      prev_signal = signal;
      switch (signal) {
        case pir_sensor_1_motion: //Serial.println("PIR Sensor 1 reports movement");
                                  //client.loop();
                                  client.publish("/all/aaron/motionDetector/pirBed/pirBed_detected", "true");
                                  //Serial.println("mqqt published");
                                  timestamp = millis();
                                  break;
      }
    }
    else{
      Serial.println("received multiple times, wrong format or '23040'");
    }

  }

  myReceiver.resetAvailable();

}
