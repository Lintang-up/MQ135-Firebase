// WIFI
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <string>

// Wifi Password
const char* ssid = "PDI";
const char* password = "69696969";

// RealTime
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000); // Waktu dalam zona WIB (UTC +7)


//Firebase
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define API_KEY "AIzaSyBknejOj3CiC1t-ALKHBMmivqRlvHigtV0"
#define DATABASE_URL "https://sensorgas-mq135-default-rtdb.asia-southeast1.firebasedatabase.app/" 
int currentNumber = 1;
unsigned long previousMillis = 0; 
const long interval = 10000; 

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

//Library Sensor asap
#include <TroykaMQ.h>
#define PIN_MQ135  A0
MQ135 mq135(PIN_MQ135);
float gs;

void setup()
{
  Serial.begin(9600);
// Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Serial.println();

// RealTime
  timeClient.begin();
  timeClient.update();

// Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

// MQ135
  mq135.calibrate();
  Serial.print("Ro = ");
  Serial.println(mq135.getRo());
}

void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    fb();
    Serial.println("______________________________");
  }
}

void senosrGas(){
  gs = mq135.readCO2();
  }

  void fb() {
  senosrGas();

  // Perbarui waktu dari NTP
  timeClient.update();

  // Dapatkan detail waktu
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();

  // Mendapatkan tanggal, bulan, tahun dari epoch time
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&rawTime);
  int currentDay = timeInfo->tm_mday;
  int currentMonth = timeInfo->tm_mon + 1; // Karena bulan dimulai dari 0, jadi tambahkan 1
  int currentYear = timeInfo->tm_year + 1900; // Tahun dimulai dari 1900, jadi tambahkan 1900

  // Format waktu dan tanggal menjadi string yang lebih informatif
  String timestamp = String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond) + "   " +
                     String(currentDay) + "-" + String(currentMonth) + "-" + String(currentYear);

  // Cek apakah currentNumber sudah ada di Firebase
  if (Firebase.RTDB.getInt(&fbdo, "/currentNumber")) {
    currentNumber = fbdo.intData();
    Serial.println("Current number retrieved: " + String(currentNumber));
  } else {
    // Jika path belum ada, inisialisasi currentNumber dengan 1
    Serial.println("Path not exist, initializing current number to 1.");
    currentNumber = 1;
    if (!Firebase.RTDB.setInt(&fbdo, "/currentNumber", currentNumber)) {
      Serial.println("FAILED to initialize current number");
      Serial.println("REASON: " + fbdo.errorReason());
      return; // Jika gagal, hentikan proses
    }
  }
  
  // Buat path berdasarkan nomor urut
  String path = "/Sensor/" + String(currentNumber);

  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.setString(&fbdo, path + "/timestamp", timestamp) &&
        Firebase.RTDB.setFloat(&fbdo, path + "/gasLevel", gs)) {

        Serial.print("Data sent to path: ");
        Serial.println(path);
        Serial.print("Tanggal: ");
        Serial.print(timestamp);
        Serial.print("  GasLevel: ");
        Serial.print(gs);
        Serial.println(" ");

        // Tingkatkan nomor urut dan simpan kembali di Firebase
        currentNumber++;
        if (!Firebase.RTDB.setInt(&fbdo, "/currentNumber", currentNumber)) {
          Serial.println("FAILED to update current number");
          Serial.println("REASON: " + fbdo.errorReason());
        }
    } 
    else {
      Serial.println("FAILED to send data");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}
