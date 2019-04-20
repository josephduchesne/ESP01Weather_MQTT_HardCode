#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <PubSubClient.h>


// Required for LIGHT_SLEEP_T delay mode
extern "C" {
#include "user_interface.h"
}

#include "config.h"

// values from config.h
char mqtt_server[64] = MQTT_SERVER;
char mqtt_port[6] = MQTT_PORT;
char mqtt_client[32] = MQTT_CLIENT;
char mqtt_user[32] = MQTT_USER;
char mqtt_pass[32] = MQTT_PASS;
char topic[128];
char msg[32];

float humidity, temp_c;  // Values read from sensor
const long interval = 60 * 1000;          // interval at which to read sensor

#include <DHT.h>
#define DHTPIN  0
DHT dht(DHTPIN, DHTTYPE, 15);  // DHTYPE is in config.h

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );
  WiFi.persistent( false );
  
  // Stop the DHT22 from messing up on boot
  digitalWrite(0, LOW); // sets output to gnd
  pinMode(0, OUTPUT); // switches power to DHT on
  delay(1000); // delay necessary after power up for DHT to stabilize

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  client.setServer(mqtt_server, atoi(mqtt_port));
}

void connect_mqtt() {
  // If not connected, then connect
  
  while (!client.connected()) {
    int err=client.connect(mqtt_client, mqtt_user, mqtt_pass);
    if (err) {
      Serial.println("connected");
    } else {
      Serial.print("q");
      Serial.println(err);
      Serial.print(" ");
      Serial.println(client.state());
      // Wait 0.1 seconds before retrying
      delay(100);
    }
  }
}

void start_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void wait_for_wifi() {
  
  Serial.print("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }

}

void publish_mqtt() {
  // Publish any mqtt topics we care about
  snprintf (topic, 127, "/sensors/%s/temp", MQTT_CLIENT);
  snprintf (msg, 31, "%.1f", temp_c);
  client.publish(topic, msg);

  snprintf (topic, 127, "/sensors/%s/humidity", MQTT_CLIENT);
  snprintf (msg, 31, "%.1f", humidity);
  client.publish(topic, msg);

  snprintf (topic, 127, "/sensors/%s/rssi", MQTT_CLIENT);
  snprintf (msg, 31, "%d", WiFi.RSSI());
  client.publish(topic, msg);
}

void get_temperature() {
  delay(1);

  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  humidity = dht.readHumidity();          // Read humidity (percent)
  temp_c = dht.readTemperature();     // Read temperature as Celcius
  // Check if any reads failed and exit early (to try again).

  if (isnan(humidity) || isnan(temp_c)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

}

void loop() {
  start_wifi();
  
  get_temperature();

  wait_for_wifi();
  connect_mqtt();
  publish_mqtt();
  client.disconnect();
  wifi_set_sleep_type(LIGHT_SLEEP_T);  // Sleep wifi

  delay(interval);
}
