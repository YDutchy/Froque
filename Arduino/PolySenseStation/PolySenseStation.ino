#include <SoftwareSerial.h>
#include <CmdMessenger.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Minor_IE";
const char* password = "Minor2017";
IPAddress server(192, 168, 2, 111);
WiFiClient espClient;
PubSubClient client(espClient);
SoftwareSerial connection(D3, D4);
CmdMessenger cmdMessenger = CmdMessenger(connection);

const int VOID = 0;
const int NOECHO = 1;
const int LUMO = 2;
const int POLYSENSE = 3;
const int IRIS = 4;

const long timeout = 500;
long timeStamp = 0;
long lastCheck = 0;
int oldRotation = 0;
boolean connexted = false;
int connextId;
char msg[50];

enum {
  kStationId,
  kConnextId,
  kConnected
};

void attachCommandCallbacks() {
  cmdMessenger.attach(kConnextId, onConnextId);
  cmdMessenger.attach(kConnected, onConnected);
}

void onConnextId() {
  connextId = cmdMessenger.readInt16Arg();
  cmdMessenger.sendCmd(kConnected);
  Serial.println("Connext " + String(connextId) + " connected");
  itoa(connextId, msg, 10);
  client.publish("polysense", msg);
}

void onConnected() {
  connexted = true;
  timeStamp = millis();
}

void setup() {
  Serial.begin(4800);
  connection.begin(9600);
  attachCommandCallbacks();
  setup_wifi();
  client.setServer(server, 1883);
  client.setCallback(callback);
}

void loop() {
  cmdMessenger.feedinSerialData();
  handleConnection();
  handleWifi();
}

void handleConnection() {
  if (!connexted) {
    cmdMessenger.sendCmd(kStationId, POLYSENSE);
  } else if (millis() - timeout > timeStamp) {
    connexted = false;
    itoa(0, msg, 10);
    client.publish("polysense", msg);
    Serial.println("Disconnected");
  } else {
    cmdMessenger.sendCmd(kConnected);
  }
  if (millis() - lastCheck > 1000 && connexted == false){
    lastCheck = millis();
    itoa(0, msg, 10);
    client.publish("polysense", msg);
  }
  delay(50);
}

void handleWifi() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("PolySenseStationClient")) {
      Serial.println("connected");
      client.publish("polysenseDebug", "Connext station online");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

