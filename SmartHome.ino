#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "pitches.h"
#include <Tone32.h>
#include <ESP32Servo.h>
#include "DHT.h"
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Flags
bool discoFlag = false;
bool melFlag = false;
bool accessGranted = false; // Tracks whether access has been granted
const char *ssid = "***"; // Enter your WiFi name
const char *password = "***";  // Enter WiFi password
const char* mqtt_server = "***.s1.eu.hivemq.cloud"; // Replace with your HiveMQ broker URL
const char* mqtt_username = "hivemq.webclient.***"; // Replace with your HiveMQ username
const char* mqtt_password = "***";
const int mqttPort = 8883;
const char *Topictemp = "atmosphere";
const char *Topiclight = "lightStatus";
const char *Topicdoor = "doorStatus";
// Pin Definitions
#define irReceiverPin 14
#define ledPin1 2       // Built-in LED pin for ESP32
#define rledPin 25      // Red LED pin
#define gledPin 26      // Green LED pin
#define bledPin 27      // Blue LED pin
#define led 22
#define BUZZER_PIN 12   // Buzzer pin
#define BUZZER_CHANNEL 0
#define LIGHT_SENSOR_PIN 35 
#define PIN_SG90 15
#define DHTPIN 33     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

unsigned long lastLdrReadTime = 0; // Tracks the last time the LDR was read
const unsigned long ldrReadInterval = 60000; // Interval in milliseconds (60 seconds)
unsigned long lastdhtReadTime = 0; // Tracks the last time the LDR was read
const unsigned long dhtReadInterval = 50000; // Interval in milliseconds (60 seconds)
// Objects
Servo sg90;
DHT dht(DHTPIN, DHTTYPE);
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin}; 
MFRC522 mfrc522{driver};

// Timing Variables
unsigned long lastSignalTime = 0;
#define signalIgnoreInterval 100 // Ignore signals within this interval (ms)
unsigned long lastDiscoUpdate = 0;

// IR Receiver Object
IRrecv IrReceiver(irReceiverPin);
decode_results results;

#define MSG_BUFFER_SIZE (50)

const char* test_root_ca= \
     "-----BEGIN CERTIFICATE-----\n" \
     "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
     "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
     "DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
     "PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
     "Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
     "AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
     "rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
     "OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
     "xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
     "7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
     "aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
     "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
     "SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
     "ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
     "AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
     "R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
     "JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
     "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
     "-----END CERTIFICATE-----\n";
  
   WiFiClientSecure espClient;
PubSubClient client(espClient);

// Callback function whenever an MQTT message is received
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++)
  {
    Serial.print(message += (char)payload[i]);
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("MQTT Broker connected!");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishMessage(const char *topic, float value)
{
  char msgBuffer[MSG_BUFFER_SIZE];
  snprintf(msgBuffer, MSG_BUFFER_SIZE, "%g", value);
  Serial.printf("Publishing to topic :: %s, value :: %s", topic, msgBuffer);
  Serial.println("");
  client.publish(topic, msgBuffer);
  //digitalWrite(LEDB,HIGH);
}

// Connect to Wifi
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to TOPNET_");
 // Serial.println(ssid);
  espClient.setCACert(test_root_ca);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
   //digitalWrite(LEDG,HIGH);

   espClient.setInsecure();
   
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin1, OUTPUT);  // Built-in LED pin
  pinMode(rledPin, OUTPUT);  // Red LED pin
  pinMode(gledPin, OUTPUT);  // Green LED pin
  pinMode(bledPin, OUTPUT);  // Blue LED pin
  pinMode(led, OUTPUT);  // Blue LED pin
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure the buzzer starts in a silent state
  analogSetAttenuation(ADC_11db);
  dht.begin();
  // Initialize components
  IrReceiver.enableIRIn();
  sg90.attach(PIN_SG90);
  sg90.write(0);
  dht.begin();
  mfrc522.PCD_Init();
     setup_wifi();
  // setup the mqtt server and callback
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);
}

void melody() {
  tone(BUZZER_PIN, NOTE_C4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

  tone(BUZZER_PIN, NOTE_C4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

  tone(BUZZER_PIN, NOTE_G4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

  tone(BUZZER_PIN, NOTE_G4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

  tone(BUZZER_PIN, NOTE_A4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

  tone(BUZZER_PIN, NOTE_A4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);

  tone(BUZZER_PIN, NOTE_G4, 650, BUZZER_CHANNEL);
  delay(750);
  noTone(BUZZER_PIN, BUZZER_CHANNEL);
}

void setColor(int R, int G, int B) {
  analogWrite(rledPin, R);  // Red channel
  analogWrite(gledPin, G);  // Green channel
  analogWrite(bledPin, B);  // Blue channel
}

void LDR() {
  const int numSamples = 10; // Number of samples to average
  int totalValue = 0;

  for (int i = 0; i < numSamples; i++) {
    Serial.println("*******");
    totalValue += analogRead(LIGHT_SENSOR_PIN);
    delay(10);
  }

  int analogValue = totalValue / numSamples;
  Serial.print("Analog Value = ");
  Serial.print(analogValue);

  if (analogValue < 50) {
    client.publish(Topiclight, "Dark");
    Serial.println("Message published successfully => Dark");
    digitalWrite(led, HIGH);
  } else if (analogValue < 1000) {
    client.publish(Topiclight, "Dim");
    Serial.println("Message published successfully => Dim");
  } else if (analogValue < 2500) {
    client.publish(Topiclight, "Light");
    Serial.println("Message published successfully => Light");
  } else if (analogValue < 3500) {
    client.publish(Topiclight, "Bright");
    Serial.println("Message published successfully => Bright");
  } else {
    client.publish(Topiclight, "Very bright");
    Serial.println("Message published successfully => Very bright");
    
  }
}

void IR() {
  if (IrReceiver.decode(&results)) {
    unsigned long decCode = results.value;
    Serial.println(decCode, HEX);

    if (millis() - lastSignalTime > signalIgnoreInterval) {
      if (decCode == 0xFF18E7) {  // Bright white light
        discoFlag = false;
        melFlag = false;
        setColor(255, 255, 255);
      } else if (decCode == 0xFF7A85) {  // Yellow light
        discoFlag = false;
        melFlag = false;
        setColor(255, 153, 0);
      } else if (decCode == 0xFF30CF) {  // Start disco mode
        discoFlag = true;
        melFlag = false;
      } else if (decCode == 0xFFA25D) {  // Stop disco mode
        discoFlag = false;
        melFlag = false;
        setColor(0, 0, 0);
      } else if (decCode == 0xFF10EF) {  // Start melody
        discoFlag = false;
        melFlag = true;
        melody();
      } else if (decCode == 0xFF629D) {  // Reset flags
        melFlag = false;
        discoFlag = false;
      }

      lastSignalTime = millis();
    }

    IrReceiver.resume();
  }

  if (discoFlag) {
    if (millis() - lastDiscoUpdate > 1000) {
      lastDiscoUpdate = millis();

      int randomR = random(0, 256);
      int randomG = random(0, 256);
      int randomB = random(0, 256);

      setColor(randomR, randomG, randomB);
    }
  }
}
void DHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if readings are valid
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor! Check wiring."));
    return;
  }

  Serial.println("****");
  Serial.printf("Humidity: %.1f%%  Temperature: %.1f°C\n", h, t);

  // Convert temperature and humidity to strings
  char temperatureStr[10];
  char humStr[10];
  dtostrf(t, 4, 1, temperatureStr);
  dtostrf(h, 4, 1, humStr);

  // Create JSON payload
  DynamicJsonDocument jsonPayload(128);
  jsonPayload["temperature"] = temperatureStr;
  jsonPayload["hum"] = humStr;

  String serializedPayload;
  serializeJson(jsonPayload, serializedPayload);
  Serial.print("Serialized Payload: ");
  Serial.println(serializedPayload);

  // Publish the payload to the MQTT topic
  if (client.publish(Topictemp, serializedPayload.c_str())) {
    Serial.println("Message published successfully!");
  } else {
    Serial.println("Failed to publish message.");
  }
}
// Global variable to track the current door state
String doorState = "closed"; // Initial state of the door

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  delay(2000); 
  unsigned long currentTime = millis();

  // RFID logic (only runs if access has not been granted)
  if (!accessGranted && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("RFID Card Detected!");

    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    Serial.print("Card UID: ");
    Serial.println(uidString);

    if (uidString.equals("FA52A115")) { // Check for authorized card
      Serial.println("Access Granted!");
      accessGranted = true;

      delay(1000); // Hold the servo at 180 degrees for 1 second

      // Open door only if it's currently closed
      if (doorState == "closed") {
        for (int pos = 180; pos >= 0; pos -= 1) {
          sg90.write(pos);
          delay(20); // Smooth movement
        }
        client.publish(Topicdoor, "Door opened");
        Serial.println("Message published successfully: Door opened");
        doorState = "opened"; // Update the door state
      }

      delay(1000); // Keep door open for a moment

      // Close door only if it's currently opened
      if (doorState == "opened") {
        for (int pos = 0; pos <= 180; pos += 1) {
          sg90.write(pos);
          delay(20);
        }
        client.publish(Topicdoor, "Door closed");
        Serial.println("Door closed");
        doorState = "closed"; 
      }
      
    } else {
      Serial.println("Not Authorized!");
    }

    delay(1000); 
  }

  // Read the LDR every minute
  if (currentTime - lastLdrReadTime >= ldrReadInterval) {
    lastLdrReadTime = currentTime;
    LDR();
  }

  // Read DHT sensor every interval
  if (currentTime - lastdhtReadTime >= dhtReadInterval) {
    lastdhtReadTime = currentTime;
    DHT();
  }

  // Execute IR functionality
  IR();
}
