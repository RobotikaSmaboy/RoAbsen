/*
 * RoAbsen - RoBoys 'Absen' machine
 *
 * Copyright(C) 2023 Robotika Smaboy
*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include "secrets.h"

// Initialize LiquidCrystal I2C object
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize RFID MFRC522
#define SDA_PIN D8
MFRC522 rfid(SDA_PIN);
MFRC522::MIFARE_Key rfid_key; 

// Initialize Wi-Fi (WifiMulti)
// TODO: Support HTTPS
// WiFiClient wifiClient;
WiFiClientSecure wifiClient;
ESP8266WiFiMulti wifiMulti;
HTTPClient httpClient;
const uint32_t wifiTimeout = 15000;

// Button pins
#define ROABSEN_ABSEN_PIN D4
#define ROABSEN_CHECK_UID_PIN D3

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    rfid_key.keyByte[i] = 0xFF;
  }

  WiFi.setHostname("RoAbsen");
  wifiMulti.addAP("Sinar Alam", "salma123");
  wifiMulti.addAP("Robotika Smaboy", "codeandbuild");
  wifiMulti.addAP("SMABOY HOTSPOT", "smaboymonumental");
  wifiClient.setInsecure();
  httpClient.useHTTP10(true);

  pinMode(ROABSEN_ABSEN_PIN, INPUT);
  pinMode(ROABSEN_CHECK_UID_PIN, INPUT);


  // Intro
  lcd.setCursor(0,0);
  lcd.print("RoAbsen v1.0");
  lcd.setCursor(0,1);
  lcd.print("(C) 2023 RoBoys");
  delay(1500);
  lcd.clear();

}

/*
 * Function: Clear one LCD line
*/
void clearLCDLine(int line)
{               
  lcd.setCursor(0,line);
  for(int n = 0; n < 16; n++) {
    lcd.print(" ");
  }
}

/*
 * Function: Stop RFID authentication
*/
void stopRFIDAuth() {
  // Needed to stop RFID authentication
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

/*
 * Function: Detect RFID Card
*/
void detectRFIDCard() {
  lcd.setCursor(0,0);
  lcd.print("Tempelkan kartu");
  lcd.setCursor(0,1);
  lcd.print("ID: ");
  while ( ! rfid.PICC_IsNewCardPresent()) {
    // rfid.PICC_IsNewCardPresent();
  }
  while ( ! rfid.PICC_ReadCardSerial()) {
    // rfid.PICC_ReadCardSerial();
  }

  clearLCDLine(0);
  lcd.setCursor(0,0);
  lcd.print("Terdeteksi!");


  stopRFIDAuth();
}

/*
 * Function: Retrieve RFID Card's unique ID (UID)
*/
String getRFIDCardUID(byte *uidByte, byte uidSize) {
  String cardUid;
  for (byte i = 0; i < uidSize; i++) {
    cardUid += uidByte[i] < 0x10 ? " 0" : " ";
    cardUid += uidByte[i];
  }
  cardUid.trim();

  Serial.println(cardUid);
  return cardUid;
}

/*
 * Function: Show RFID Card's unique ID (UID) to LCD
*/
void showRFIDCardUID(String cardUid) {
  lcd.setCursor(4,1);
  lcd.print(cardUid);
}

/*
 * Function: Send HTTP request for Absen
*/
void sendAbsenRequest(String cardUid) {
  StaticJsonDocument<48> apiJson;
  StaticJsonDocument<192> apiResponse;
  String json;

  apiJson["card_uid"] = cardUid;
  serializeJson(apiJson, json);

  httpClient.begin(wifiClient, (String)ROBOYS_API_ABSEN_URL);
  httpClient.addHeader("Content-Type", "application/json");
  httpClient.setAuthorization(ROBOYS_API_USERNAME, ROBOYS_API_PASSWORD);
  int req = httpClient.POST(json);

  deserializeJson(apiResponse, httpClient.getString());
  if (req != HTTP_CODE_OK || apiResponse["message"]) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Error:");
    lcd.setCursor(0,1);
    lcd.print(apiResponse["message"].as<String>());
  } else {
    clearLCDLine(0);
    lcd.setCursor(0,0);
    lcd.print("Sukses!");
    lcd.setCursor(0,1);
    lcd.print(apiResponse["nama"].as<String>());
  }
  delay(3500);

  // Disconnect
  httpClient.end();
}

/*
 * Menu: Check RFID Card's unique ID (UID)
*/
void checkUID() {
  detectRFIDCard();
  String cardUid = getRFIDCardUID(rfid.uid.uidByte, rfid.uid.size);
  showRFIDCardUID(cardUid);
  delay(5000);

  lcd.clear();
  checkUID();
}

/*
 * Menu: Absen
*/
void absen() {
  detectRFIDCard();
  String cardUid = getRFIDCardUID(rfid.uid.uidByte, rfid.uid.size);
  showRFIDCardUID(cardUid);
  delay(500);

  clearLCDLine(0);
  lcd.setCursor(0,0);
  lcd.print("Proses Absensi...");
  sendAbsenRequest(cardUid);

  lcd.clear();
  absen();
}


void loop(){
  // Always stop RFID
  stopRFIDAuth();

  // Connect Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(0,0);
    lcd.print("Connecting");
    lcd.setCursor(0,1);
    lcd.print("Wi-Fi");
    while (wifiMulti.run(wifiTimeout) != WL_CONNECTED) {
        lcd.print(".");
        delay(500);
    }
    lcd.clear();
  }

  if(digitalRead(ROABSEN_ABSEN_PIN) == LOW) {
    lcd.clear();
    absen();
  } else if(digitalRead(ROABSEN_CHECK_UID_PIN) == LOW) {
    lcd.clear();
    checkUID();
  }

  lcd.setCursor(0,0);
  lcd.print("1) Absensi Siswa");
  lcd.setCursor(0,1);
  lcd.print("2) Lihat UID");
}
