/*
  Basic ESP8266 MQTT example

  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.

  It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.

  To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <PubSubClient.h>

// Update these with values suitable for your network.

//const char* ssid = "Minor_IE";
//const char* password = "Minor2017";
IPAddress server(192, 168, 2, 111);

WiFiClient espClient;
PubSubClient client(espClient);
long lastChange = 0;
const long personalTimeout = 100;
char debugMsg[50];
int value = 0;

const byte dataPins[4] = {D1, D2, D3, D4}; // connect to {2, 3, 4, 5} on acturator Arduino
bool dataState[4];

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

  for (int i = 0; i < 4; i++) {
    pinMode(dataPins[i], OUTPUT);
    digitalWrite(dataPins[i], HIGH);
  }

  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.autoConnect("Polysense", "tunnel");


  //setup_wifi();
  client.setServer(server, 1883);
  client.setCallback(callback);

}
/*
  void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  //Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  }
*/
void callback(char* topic, byte* payload, unsigned int length) {

  String inString = "";

  Serial.print("Message arrived [ID: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);

    if (isDigit(payload[i])) {
      inString += (char)payload[i];
    }
  }

  Serial.println("]");

  snprintf(debugMsg, 75, "Recieved ID:%ld", inString.toInt());
  client.publish("polysenseDebug", debugMsg);

  if (dataState[0] == false) {
    dataState[0] = true;
    lastChange = millis();

    switch (inString.toInt()) {
      case 6:
        dataState[1] = false;
        dataState[2] = false;
        dataState[3] = false;
        break;
      case 8:
        dataState[1] = true;
        dataState[2] = false;
        dataState[3] = false;
        break;
      case 14:
        dataState[1] = false;
        dataState[2] = true;
        dataState[3] = false;
        break;
      case 22:
        dataState[1] = true;
        dataState[2] = true;
        dataState[3] = false;
        break;
      case 49:
        dataState[1] = false;
        dataState[2] = false;
        dataState[3] = true;
        break;
      case 66:
        dataState[1] = true;
        dataState[2] = false;
        dataState[3] = true;
        break;
      case 75:
        dataState[1] = false;
        dataState[2] = true;
        dataState[3] = true;
        break;
      case 90:
        dataState[1] = true;
        dataState[2] = true;
        dataState[3] = true;
        break;
      default:
        dataState[0] = false;
        client.publish("polysenseDebug", "unknown ID");
        break;
    }

  } else if (inString.toInt() == 0) {
    dataState[0] = false;
    client.publish("polysenseDebug", "disconnext");
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("PolysensClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("polysenseDebug", "Online");
      // ... and resubscribe
      client.subscribe("polysense");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  /*
    if (millis() - lastChange > personalTimeout) {
      dataState[0] = false;
    }*/

  client.loop();

  for (int i = 0; i < 4; i++) {
    digitalWrite(dataPins[i], dataState[i]);
  }


  /*
    long now = millis();
    if (now - lastMsg > 2000) {
      lastMsg = now;
      ++value;
      snprintf (msg, 75, "hello world #%ld", value);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("outTopic", msg);
    }
  */
}

