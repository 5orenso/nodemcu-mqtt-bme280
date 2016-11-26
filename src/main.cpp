/*
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BME280.h>

#define SLEEP_DELAY_IN_SECONDS  30
#define DEFAULT_AIR_UPDATE_INTERVAL_MS 1111

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const char* mqtt_server = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;

const char* outTopic = MQTT_OUT_TOPIC;
const char* inTopic = MQTT_IN_TOPIC;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[150];
int chipId = ESP.getChipId();

uint32_t airUpdateIntervalMs = DEFAULT_AIR_UPDATE_INTERVAL_MS;
unsigned long lastAirUpdateMillis = 0;

BME280 Air; // SDA = D2, SCL = D1

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("."); Serial.print(ssid);
        delay(500);
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
        // it is acive low on the ESP-01)
    } else { }
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
            // client.publish(outTopic, "{ \"chipId\": chipId, \"ping\": \"hello world\" }");
            // ... and resubscribe
            client.subscribe(inTopic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

static void setupAirSensor(void) {
    if (!Air.begin()) {
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    }
}

void setup(void) {
    Serial.begin(115200);
    setupAirSensor();
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    float value = 0.0;
    char temperatureValue[10];
    char pressureValue[10];
    char humidityValue[10];
    long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;
        float temperature = Air.ReadTemperature();
        float pressure = Air.ReadPressure() / 100;
        float humidity = Air.ReadHumidity();
        dtostrf(temperature, 3, 2, temperatureValue); // first 2 is the width including the . (1.) and the 2nd 2 is the precision (.23)
        dtostrf(pressure, 4, 1, pressureValue);
        dtostrf(humidity, 3, 0, humidityValue);

        snprintf (msg, 150, "{ \"chipId\": %d, \"temperature\": %s, \"pressure\": %s, \"humidity\": %s }", chipId, temperatureValue, pressureValue, humidityValue);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish(outTopic, msg);
        Serial.print("Entering deep sleep mode for ");
        Serial.print(SLEEP_DELAY_IN_SECONDS);
        Serial.println(" seconds...");
        ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
    }
}

