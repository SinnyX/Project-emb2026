#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6p-I1HYkE"
#define BLYNK_TEMPLATE_NAME "Emb"
#define BLYNK_AUTH_TOKEN "lTIJY_r_HsIMMhC6gXhsl328YHqSbSRX"
#include <BlynkSimpleEsp32.h>
BlynkTimer timer;

// Firebase configuration
#include <WiFi.h>
#include <FirebaseESP32.h>
#define FIREBASE_HOST "project-emb-a6725-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyCFH00MqZBkP6sUacCclm4HFU6BbLm9G0w"
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
FirebaseJson json;

// FastAPI configuration
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define FASTAPI_SERVER "https://fastapi-lightgbm-deploy-6hef.vercel.app"
HTTPClient http;

#define ULTRA_TRINGPIN 25
#define ULTRA_ECHOPIN 26

#define MICROPIN 33
#define RELAYPIN 14
#define FAN_INAPIN 18
#define FAN_INBPIN 19
#define PHOTOPIN 32
#define FALMEPIN 34

#include "DHT.h"
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

double value_humidity, value_temp, value_photo, value_flame;
int value_ultra, value_micro;
bool state = 0;

void Flame_Sensor();
void Micro_Sensor();
void Ultra_Sensor();
long Ultra_microsecondsToCentimeters(long microseconds);
void DHT_Sensor();
void Photo_Sensor();
void Fan_on();
void Fan_off();
void Relay_on();
void Relay_off();
void sendDataToFirebase();
bool predictMotorState();

BLYNK_WRITE(V6) {
  int pinValue = param.asInt();
  Serial.print(F("Motor= "));
  Serial.println(pinValue);

  // process received value
  if (pinValue == 1) {
    Relay_on();
  } else {
    Relay_off();
  }
}

void myTimerEvent() {
  Blynk.virtualWrite(V0, value_humidity);
  Blynk.virtualWrite(V1, value_temp);
  Blynk.virtualWrite(V2, value_photo);

  Blynk.virtualWrite(V3, value_ultra);
  Blynk.virtualWrite(V4, value_micro);
  Blynk.virtualWrite(V6, state);
  Blynk.virtualWrite(V7, value_flame);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable detector
  Serial.begin(115200);
  // Blynk.begin(BLYNK_AUTH_TOKEN, "Poomphasin 2.4G", "0956579194");
  Blynk.begin(BLYNK_AUTH_TOKEN, "iPhonePanunan", "@1234567890");
  timer.setInterval(1000L, myTimerEvent);

  // Initialize Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  dht.begin();  //DHT
  pinMode(FAN_INAPIN, OUTPUT);
  pinMode(FAN_INBPIN, OUTPUT);
  pinMode(RELAYPIN, OUTPUT);

  pinMode(ULTRA_TRINGPIN, OUTPUT);
  pinMode(ULTRA_ECHOPIN, OUTPUT);
  pinMode(ULTRA_ECHOPIN, OUTPUT);

  pinMode(FALMEPIN, INPUT);
  pinMode(MICROPIN, INPUT);
  pinMode(PHOTOPIN, INPUT);
}

void loop() {
  Blynk.run();
  timer.run();
  Blynk.syncVirtual(V6);
  // Wait a few seconds between measurements.
  delay(2000);

  Ultra_Sensor();
  Micro_Sensor();

  // Use LightGBM prediction via FastAPI instead of if-else
  bool shouldOpen = predictMotorState();
  
  if (shouldOpen) {
    Serial.print(F(", Result: Open (ML Prediction)"));
    if (value_ultra < 30) {
      Blynk.logEvent("motor_open", "Motor is opening!");
    }
    toggleState();
  } else {
    Serial.print(F(", Result: Close (ML Prediction)"));
  }
  Serial.println();
  DHT_Sensor();
  Photo_Sensor();
  Flame_Sensor();
  
  // Send data to Firebase
  sendDataToFirebase();
}

void Micro_Sensor() {
  value_micro = analogRead(MICROPIN);
  Serial.print(F("Microphone: "));
  Serial.print(value_micro);
}

void Ultra_Sensor() {
  long duration, cm;

  digitalWrite(ULTRA_TRINGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRA_TRINGPIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(ULTRA_TRINGPIN, LOW);
  pinMode(ULTRA_ECHOPIN, INPUT);
  duration = pulseIn(ULTRA_ECHOPIN, HIGH);

  cm = Ultra_microsecondsToCentimeters(duration);
  if (isnan(cm)) {
    Serial.println(F("Failed to read from Ultra sensor!"));
    return;
  }
  value_ultra = cm;
  Serial.print(F("Ultrasonic: "));
  Serial.print(cm);
  Serial.print("cm, ");
}

long Ultra_microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

void Relay_on() {
  digitalWrite(RELAYPIN, HIGH);
}

void Relay_off() {
  digitalWrite(RELAYPIN, LOW);
}

void Fan_on() {
  digitalWrite(FAN_INAPIN, LOW);
  digitalWrite(FAN_INBPIN, HIGH);
}

void Fan_off() {
  digitalWrite(FAN_INAPIN, LOW);
  digitalWrite(FAN_INBPIN, LOW);
}

void Photo_Sensor() {
  float v = analogRead(PHOTOPIN);
  value_photo = v;
  if (isnan(v)) {
    Serial.println(F("Failed to read from Photo sensor!"));
    return;
  }
  Serial.print(F("Photo: "));
  Serial.println(v);
}


void DHT_Sensor() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  value_humidity = h;
  value_temp = t;

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
}

void Flame_Sensor() {
  int v = analogRead(FALMEPIN);
  value_flame = v;
  if (isnan(v)) {
    Serial.println(F("Failed to read from Flame sensor!"));
    return;
  }
  Serial.print(F("Flame: "));
  Serial.println(v);
}

void toggleState() {
  state = !state;
}

void sendDataToFirebase() {
  // Create JSON object with all sensor data
  json.clear();
  json.set("humidity", value_humidity);
  json.set("temperature", value_temp);
  json.set("photo", value_photo);
  json.set("ultrasonic", value_ultra);
  json.set("microphone", value_micro);
  json.set("flame", value_flame);
  json.set("relay_state", state);
  json.set("timestamp", millis());

  // Send data to Firebase
  if (Firebase.setJSON(firebaseData, "/sensors", json)) {
    Serial.println("Data sent to Firebase successfully");
  } else {
    Serial.print("Firebase error: ");
    Serial.println(firebaseData.errorReason());
  }
}

bool predictMotorState() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, using fallback logic");
    // Fallback to original logic if WiFi is not connected
    return (value_ultra < 30 && value_micro > 2000);
  }

  http.begin(FASTAPI_SERVER "/predict");
  http.addHeader("Content-Type", "application/json");
  
  // For HTTPS, skip certificate validation (not recommended for production)
  // For production, add proper SSL certificate
  #ifdef ESP32
    http.setInsecure(); // Skip SSL certificate validation for Vercel
  #endif

  // Create JSON payload - Vercel API expects only "feature" field
  // Using ultrasonic value as the feature
  StaticJsonDocument<100> doc;
  doc["feature"] = value_ultra;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.print("Sending to FastAPI: ");
  Serial.println(jsonPayload);

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("FastAPI Response: ");
    Serial.println(response);

    // Parse JSON response
    StaticJsonDocument<200> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);

    if (!error) {
      // Check if response has error field
      if (responseDoc.containsKey("error")) {
        Serial.print("API Error: ");
        Serial.println(responseDoc["error"].as<String>());
        if (responseDoc.containsKey("detail")) {
          Serial.print("Detail: ");
          Serial.println(responseDoc["detail"].as<String>());
        }
        http.end();
        // Fallback to original logic
        return (value_ultra < 30 && value_micro > 2000);
      }
      
      // Normal response
      int prediction = responseDoc["prediction"];
      float probability = responseDoc["probability"];
      String message = responseDoc["message"];

      Serial.print("Prediction: ");
      Serial.print(message);
      Serial.print(" (Probability: ");
      Serial.print(probability);
      Serial.println(")");

      http.end();
      return (prediction == 1);
    } else {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
      http.end();
      // Fallback to original logic
      return (value_ultra < 30 && value_micro > 2000);
    }
  } else {
    Serial.print("FastAPI request failed, error code: ");
    Serial.println(httpResponseCode);
    http.end();
    // Fallback to original logic
    return (value_ultra < 30 && value_micro > 2000);
  }
}
