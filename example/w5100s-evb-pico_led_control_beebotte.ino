#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define LEDPIN 25  // Built-in LED pin of W5100S-EVB-Pico.

#define BBT "mqtt.beebotte.com"         // Domain name of Beebotte MQTT service
#define TOKEN "token_xxxxxxxxxxxxxxxx"  // Set your channel token here

#define CHANNEL "W5100SEVBPico"  // Replace with your channel name
#define LED_RESOURCE "led"

// Network configration
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(xxx, xxx, xxx, xxx);
IPAddress myDns(xxx, xxx, xxx, xxx);
IPAddress gateway(xxx, xxx, xxx, xxx);
IPAddress subnet(xxx, xxx, xxx, xxx);

EthernetClient ethClient;
PubSubClient client(ethClient);

// to track delay since last reconnection
long lastReconnectAttempt = 0;

const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

char id[17];

const char* generateID() {
  randomSeed(analogRead(0));
  int i = 0;
  for (i = 0; i < sizeof(id) - 1; i++) {
    id[i] = chars[random(sizeof(chars))];
  }
  id[sizeof(id) - 1] = '\0';

  return id;
}

// will be called every time a message is received
void onMessage(char* topic, byte* payload, unsigned int length) {

  // decode the JSON payload
  StaticJsonDocument<256> doc;

  // Test if parsing succeeded
  auto error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }

  // led resource is a boolean read it accordingly
  bool data = doc["data"];

  // Print the received value to serial monitor for debugging
  Serial.print("Received message of length ");
  Serial.print(length);
  Serial.println();
  Serial.print("data ");
  Serial.print(data);
  Serial.println();

  // Set the led pin to high or low
  if (data == 0) {
    digitalWrite(LEDPIN, LOW);
  } else if (data == 1) {
    digitalWrite(LEDPIN, HIGH);
  }
  // digitalWrite(LEDPIN, data ? HIGH : LOW);
}

// reconnects to Beebotte MQTT server
boolean reconnect() {
  if (client.connect(generateID(), TOKEN, "")) {
    char topic[64];
    sprintf(topic, "%s/%s", CHANNEL, LED_RESOURCE);
    client.subscribe(topic);

    Serial.println("Connected to Beebotte MQTT");
  }
  return client.connected();
}

void setup() {
  pinMode(LEDPIN, OUTPUT);

  client.setServer(BBT, 1883);
  // Set the on message callback
  // onMesage function will be called
  // every time the client received a message
  client.setCallback(onMessage);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Ethernet.init(17);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using static IP address instead of DHCP:
    // Feel free to change this according to your network settings
    // IPAddress ip(192, 168, 56, 177);
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("connecting...");
  lastReconnectAttempt = 0;
}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }
}