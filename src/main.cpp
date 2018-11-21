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
int prev_signal = 0;
unsigned long timestamp = 0;

//transmit variables
//...

/* MQTT subscriptions
- /all/aaron/bridge/433/433_transmit_command
 */

 /* MQTT publish
 - /all/aaron/bridge/433/433_receive_command
  */


  // void declaration
  void setup_wifi();
  void reconnect();

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;




void callback(char* topic, byte* payload, unsigned int length) {

  String tp = topic;
  char message[] = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i] = (char)payload[i];
  }

  int message_int = atoi(message);


  Serial.println();

/*
  if (tp.indexOf("/all/aaron/bridge/ir/ir_transmit_command")>=0) {
    char send_code[] = "";
    send_code[0] = ir_remote[0];
    send_code[1] = ir_remote[1];
    for (size_t i = 2; i < length+2; i++) {
      send_code[i] = payload[i];
    }

    mySwitch.sendTriState(send_code);
    Serial.print("IR send: ");
    Serial.println(send_code);

  }
*/
  if (tp.indexOf("/all/aaron/bridge/433/433_transmit_command")>=0) {
    myReceiver.disableReceive();
    mySwitch.send(message_int, 24);
    myReceiver.enableReceive(receiver);
    Serial.print("433 send: ");
    Serial.println(message_int);
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
  //mySwitch.send(23600,24);
  Serial.println("setup finished");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();



  if (myReceiver.available()) {

    int signal = myReceiver.getReceivedValue();
    char signal_c[] = "";
    itoa(signal, signal_c, 10);

    Serial.println("");
    Serial.print("message received: ");
    Serial.println(signal);

    if ((!(signal == prev_signal) || millis() - timestamp > 2000) && (signal > 0)){
      prev_signal = signal;
      client.publish("/all/aaron/bridge/433/433_receive_command", signal_c, sizeof(signal_c));
    }
    else{
      Serial.println("received multiple times, wrong format or '23040'");
    }

  }

  myReceiver.resetAvailable();

}

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
      client.publish("/all/aaron/bridge/433/433_transmit_command", "0");
      client.publish("/all/aaron/bridge/433/433_receive_command", "0");
      // ... and resubscribe
      client.subscribe("/all/aaron/bridge/433/433_transmit_command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
