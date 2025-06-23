// --- Include Library ---
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"      // Firebase Token Handler
#include "addons/RTDBHelper.h"       // Firebase Realtime DB Handler

// --- Konfigurasi WiFi ---
#define WIFI_SSID "ESP32WIFI"
#define WIFI_PASSWORD "12345678"

// --- Konfigurasi Firebase ---
#define FIREBASE_AUTH "AIzaSyA_inSMQwOXNXB4lXfo5XKA6C_lNcE_hls"
#define FIREBASE_HOST "https://dsystem-15d0e-default-rtdb.asia-southeast1.firebasedatabase.app"

// --- Pin Sensor & Aktuator ---
#define LDR_PIN 34           // Pin LDR (analog)
#define LED_LDR_PIN 27       // LED indikator LDR
#define MQ2_AO_PIN 35        // Sensor Gas MQ-2 (analog)
#define BUZZER_PIN 26        // Buzzer aktif
#define TRIG_PIN 5           // HC-SR04 - Trig
#define ECHO_PIN 18          // HC-SR04 - Echo
#define POMPA_LED_PIN 2      // Simulasi pompa (pakai LED)

FirebaseData fbdo;           // Objek Firebase untuk baca/tulis
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200); // Debug Serial Monitor

  // Set pin output
  pinMode(LED_LDR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(POMPA_LED_PIN, OUTPUT);

  // Koneksi ke WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi Terhubung");

  // Inisialisasi Firebase
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;

  // Signup anonim ke Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase SignUp Gagal: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Jika belum ada kontrol mode di database, set default
  if (Firebase.ready() && signupOK) {
    if (!Firebase.RTDB.getString(&fbdo, "control/mode")) {
      Firebase.RTDB.setString(&fbdo, "control/mode", "auto");
    }
    Firebase.RTDB.setBool(&fbdo, "control/pompa", false);
    Firebase.RTDB.setBool(&fbdo, "control/led", false);
  }
}

void loop() {
  // --- Baca Sensor LDR ---
  int ldrValue = analogRead(LDR_PIN);
  String ldrStatus = (ldrValue > 2900) ? "Malam → Lampu Menyala💡" : "Siang → Lampu Mati";

  // --- Baca Sensor Gas MQ-2 ---
  int gasValue = analogRead(MQ2_AO_PIN);
  String gasStatus = (gasValue > 1000) ? "GAS TERDETEKSI ⚠️" : "Gas Aman ✅";

  // Aktifkan buzzer jika gas terdeteksi
  if (gasValue > 1000) tone(BUZZER_PIN, 1000);
  else noTone(BUZZER_PIN);

  // --- Baca Sensor Ultrasonik (Jarak air) ---
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2.0;  // dalam cm
  String waterStatus = (distance > 15) ? "Air Rendah! Pompa Menyala ⚠️" : "Air Cukup ✅";

  // --- Kirim data sensor ke Firebase (setiap 5 detik) ---
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    Firebase.RTDB.setInt(&fbdo, "sensors/ldr/value", ldrValue);
    Firebase.RTDB.setString(&fbdo, "sensors/ldr/status", ldrStatus);
    Firebase.RTDB.setInt(&fbdo, "sensors/gas/value", gasValue);
    Firebase.RTDB.setString(&fbdo, "sensors/gas/status", gasStatus);
    Firebase.RTDB.setFloat(&fbdo, "sensors/water/distance", distance);
    Firebase.RTDB.setString(&fbdo, "sensors/water/status", waterStatus);
  }

  // --- Ambil mode kontrol dari Firebase ---
  if (Firebase.ready() && signupOK) {
    String mode = "auto";
    if (Firebase.RTDB.getString(&fbdo, "control/mode")) {
      mode = fbdo.stringData();
    }

    if (mode == "auto") {
      // === Mode Otomatis ===
      digitalWrite(LED_LDR_PIN, (ldrValue > 2900) ? HIGH : LOW);        // Gelap → nyalakan LED
      digitalWrite(POMPA_LED_PIN, (distance > 15) ? HIGH : LOW);       // Air rendah → nyalakan pompa
      if (gasValue > 1600) tone(BUZZER_PIN, 1000);                     // Gas terdeteksi → bunyikan buzzer
      else noTone(BUZZER_PIN);
    } else {
      // === Mode Manual ===
      // LED Manual
      if (Firebase.RTDB.getBool(&fbdo, "control/led")) {
        digitalWrite(LED_LDR_PIN, fbdo.boolData() ? HIGH : LOW);
      }

      // Pompa Manual
      if (Firebase.RTDB.getBool(&fbdo, "control/pompa")) {
        digitalWrite(POMPA_LED_PIN, fbdo.boolData() ? HIGH : LOW);
      }

      // Buzzer Manual
      if (Firebase.RTDB.getBool(&fbdo, "control/buzzer")) {
        if (fbdo.boolData()) tone(BUZZER_PIN, 1000);
        else noTone(BUZZER_PIN);
      }
    }
  }

  // --- Output ke Serial Monitor untuk debug ---
  Serial.println("+------------+----------------+------------------------------+");
  Serial.printf("| LDR        | %-14d | %s\n", ldrValue, ldrStatus.c_str());
  Serial.printf("| MQ-2       | %-14d | %s\n", gasValue, gasStatus.c_str());
  Serial.printf("| HC-SR04    | %-14.2f | %s\n", distance, waterStatus.c_str());
  Serial.println("+------------+----------------+------------------------------+\n");

  delay(5000); // Delay 5 detik sebelum mengulang loop
}
