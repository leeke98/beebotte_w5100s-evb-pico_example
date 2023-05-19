#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

# define LIGHT_SENSOR_PIN   26 // GP26

#define BBT "mqtt.beebotte.com"        // Domain name of Beebotte MQTT service
#define TOKEN "token_xxxxxxxxxxxxxxxx" // Set your channel token here

#define CHANNEL "W5100SEVBPicoBoard"   // Replace with your channel name
#define LIGHT_RESOURCE "light_value"   // Replace with your resource name

#define WRITE true // indicate if published data should be persisted or not

// Enter your network setting
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(000, 000, 000, 000);
IPAddress myDns(000, 000, 000, 000);
IPAddress gateway(000, 000, 000, 000);
IPAddress subnet(000, 000, 000, 000);

EthernetClient ethClient;
PubSubClient client(ethClient);

// will store last time sensor data was read
unsigned long lastReadingMillis = 0;

// interval for sending sensor vlaue to Beebotte
const long interval = 10000;
// to track delay since last reconnection
unsigned long lastReconnectAttempt = 0;

const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

char id[17]; // Identifier for the MQTT connection - will set it randomly

const char * generateID()
{
  randomSeed(analogRead(0));
  int i = 0;
  for(i = 0; i < sizeof(id) - 1; i++) {
    id[i] = chars[random(sizeof(chars))];
  }
  id[sizeof(id) -1] = '\0';

  return id;
}

// Publish MQTT Messages
// Upgraded by using ChatGPT
void publish(const char* resource, float data, bool persist)
{
  DynamicJsonDocument jsonOutBuffer(128);
  JsonObject root = jsonOutBuffer.to<JsonObject>();
  root["channel"] = CHANNEL;
  root["resource"] = resource;
  if (persist) {
    root["write"] = true;
  }
  root["data"] = data;

  // Now serialize the JSON into a string
  String jsonString;
  serializeJson(root, jsonString);

  // Create the topic to publish to
  char topic[64];
  snprintf(topic, sizeof(topic), "%s/%s", CHANNEL, resource);

  // Now publish the string to Beebotte
  client.publish(topic, jsonString.c_str());
}

void readSensorData()
{
  int analogValue = analogRead(LIGHT_SENSOR_PIN);

  Serial.print("Analog reading: ");
  Serial.println(analogValue);   // the raw analog reading

  // Check if any reads failed and exit early (to try again).
  if (isnan(analogValue)) {
    Serial.println(F("Failed to read from light sensor!"));
    return;
  }

  if (!isnan(analogValue)) {
    publish(LIGHT_RESOURCE, analogValue, WRITE);
  }
}

// reconnects to Beebotte MQTT server
boolean reconnect() {
  if (client.connect(generateID(), TOKEN, "")) {
    Serial.println("Connected to Beebotte MQTT");
  }
  return client.connected();
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // dht.begin();
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  client.setServer(BBT, 1883);

  Ethernet.init(17);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }

  // give the Ethernet shield a second to initialize:
  delay(1000);

  Serial.println("connecting...");
  lastReconnectAttempt = 0;
}

void loop()
{
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    // read sensor data every 10 seconds
    // and publish values to Beebotte
    unsigned long currentMillis = millis();
    if (currentMillis - lastReadingMillis >= interval) {
      // save the last time we read the sensor data
      lastReadingMillis = currentMillis;

      readSensorData();
    }
    client.loop();
  }
}
