/*
 * RoAbsen - RoBoys 'Absen' machine
 *
 * Copyright(C) 2023 Robotika Smaboy
*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// Initialize LiquidCrystal I2C object
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize RFID MFRC522
#define RST_PIN D1
#define SDA_PIN D8
MFRC522 rfid(SDA_PIN);
MFRC522::MIFARE_Key rfid_key; 

// Button pins
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
 * Function: Detect RFID Card
*/
void detectRFIDCard() {
  lcd.setCursor(0,0);
  lcd.print("Masukkan kartu");
  while ( ! rfid.PICC_IsNewCardPresent()) {}
  while ( ! rfid.PICC_ReadCardSerial()) {}

  lcd.setCursor(0,1);
  lcd.print("Terdeteksi!");
  delay(500);
  lcd.clear();


  // Needed to stop RFID authentication
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
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

  return cardUid;
}

/*
 * Menu: Check RFID Card's unique ID (UID)
*/
void checkUID() {
  detectRFIDCard();
  
  String cardUid = getRFIDCardUID(rfid.uid.uidByte, rfid.uid.size);
  lcd.setCursor(0,0);
  lcd.print("Card UID:");
  lcd.setCursor(0,1);
  lcd.print(cardUid);
  delay(5000);
  lcd.clear();

  // LOOP
  checkUID();
}


void loop(){
  if(digitalRead(ROABSEN_CHECK_UID_PIN) == LOW) {
    clearLCDLine(0);
    delay(500);
    lcd.clear();

    checkUID();
  }

  lcd.setCursor(0,0);
  lcd.print("1) Absensi Siswa");
  lcd.setCursor(0,1);
  lcd.print("2) Lihat UID");
}
