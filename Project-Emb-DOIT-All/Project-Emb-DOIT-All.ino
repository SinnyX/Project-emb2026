#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6p-I1HYkE"
#define BLYNK_TEMPLATE_NAME "Emb"
#define BLYNK_AUTH_TOKEN "lTIJY_r_HsIMMhC6gXhsl328YHqSbSRX"
#include <BlynkSimpleEsp32.h>
BlynkTimer timer;

#define ULTRA_TRINGPIN 25
#define ULTRA_ECHOPIN 26

#define MICROPIN 33
#define RELAYPIN 12
#define FAN_INAPIN 14
#define FAN_INBPIN 39
#define PHOTOPIN 32
#define FALMEPIN 35

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
  Blynk.begin(BLYNK_AUTH_TOKEN, "Poomphasin 2.4G", "0956579194");
  timer.setInterval(1000L, myTimerEvent);

  dht.begin();  //DHT
  pinMode(FAN_INAPIN, OUTPUT);
  pinMode(FAN_INBPIN, OUTPUT);
  pinMode(RELAYPIN, OUTPUT);

  pinMode(ULTRA_TRINGPIN, OUTPUT);
  pinMode(ULTRA_ECHOPIN, OUTPUT);
  pinMode(ULTRA_ECHOPIN, OUTPUT);
  pinMode(FALMEPIN, OUTPUT);

  pinMode(MICROPIN, INPUT);
  pinMode(PHOTOPIN, INPUT);
}

void loop() {
  Blynk.run();
  timer.run();
  // Wait a few seconds between measurements.
  delay(2000);

  Ultra_Sensor();
  Micro_Sensor();

  if (value_ultra < 30 && value_micro > 2000) {
    Serial.print(F(", Result: Open"));
    toggleState();
  } else {
    Serial.print(F(", Result: Close"));
  }
  Serial.println();
  DHT_Sensor();
  Photo_Sensor();
  Flame_Sensor();
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
  Serial.print(F("째C "));
  Serial.print(f);
  Serial.print(F("째F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("째C "));
  Serial.print(hif);
  Serial.println(F("째F"));
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