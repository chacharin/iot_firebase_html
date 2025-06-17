/*
  โปรเจกต์: ESP32 ส่งสถานะปุ่มกดไปยัง Firebase ด้วยเมธอด PUT
  ผู้จัดทำ: สุดยอดนักเขียนโค้ด ESP32 (ปรับปรุงตามโจทย์)
  
  หน้าที่:
  1. เชื่อมต่อ Wi-Fi
  2. อ่านสถานะของปุ่มกดที่ต่ออยู่กับ Pin ที่กำหนด (GPIO 13)
  3. เมื่อสถานะของปุ่มมีการเปลี่ยนแปลง (จากปล่อยเป็นกด หรือ กดเป็นปล่อย)
     - ถ้าปุ่มถูกกด: ส่ง (PUT) ค่า "1" ไปยัง Firebase Realtime Database
     - ถ้าปุ่มถูกปล่อย: ส่ง (PUT) ค่า "0" ไปยัง Firebase Realtime Database
  4. การส่งข้อมูลจะเกิดขึ้นเฉพาะตอนที่สถานะเปลี่ยนเท่านั้น เพื่อลดภาระของเครือข่าย
*/

// --- 1. ไลบรารีที่จำเป็น ---
#include <WiFi.h>          // สำหรับการเชื่อมต่อ Wi-Fi
#include <HTTPClient.h>    // สำหรับการส่ง request ไปยัง Firebase

// --- 2. กรุณาแก้ไขข้อมูลในส่วนนี้ ---
const char* ssid = "MARA1";      // ใส่ชื่อ Wi-Fi (SSID) ของคุณ
const char* password = "MARAMARA1";  // ใส่รหัสผ่าน Wi-Fi ของคุณ

// URL ของ Firebase Realtime Database (ต้องลงท้ายด้วย .json และมี auth token)
// ข้อมูลจะถูกเขียนทับที่ path /test
String firebaseUrl = "https://chacharin-b70b7-default-rtdb.asia-southeast1.firebasedatabase.app/button.json?auth=9w1FYYpcfTRHGAm9grMcFTCisi1ZdK5B4E4Mzl9M";

// --- 3. การตั้งค่าฮาร์ดแวร์ ---
const int buttonPin = 13;    // Pin ที่ต่อกับปุ่มกด

// --- 4. ตัวแปรโกลบอล ---
// ตัวแปรสำหรับเก็บสถานะล่าสุดของปุ่ม (HIGH = ไม่ได้กด, LOW = กด)
// ตั้งค่าเริ่มต้นเป็น HIGH (สถานะปกติของ INPUT_PULLUP)
int lastButtonState = HIGH; 

void setup() {
  Serial.begin(115200);

  // --- ตั้งค่าการเชื่อมต่อ Wi-Fi ---
  WiFi.begin(ssid, password);
  Serial.print("กำลังเชื่อมต่อ Wi-Fi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nเชื่อมต่อ Wi-Fi สำเร็จแล้ว!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // --- ตั้งค่า Pin ของปุ่มกด ---
  // ตั้งค่าเป็น INPUT_PULLUP ทำให้ Pin มีสถานะเป็น HIGH เมื่อไม่ได้กด
  // และจะเปลี่ยนเป็น LOW เมื่อปุ่มถูกกด (ต่อกับ GND)
  pinMode(buttonPin, INPUT_PULLUP);
  
  // อ่านสถานะเริ่มต้นของปุ่มเก็บไว้
  lastButtonState = digitalRead(buttonPin);
  
  Serial.println("ตั้งค่าปุ่มกดเรียบร้อย");
  Serial.println("รอการกดปุ่ม...");
  Serial.println("------------------------------------");
}

void loop() {
  // อ่านสถานะปัจจุบันของปุ่มกด
  int currentButtonState = digitalRead(buttonPin);

  // ตรวจสอบว่าสถานะของปุ่มมีการเปลี่ยนแปลงหรือไม่
  if (currentButtonState != lastButtonState) {
    
    Serial.println("ตรวจพบการเปลี่ยนแปลงสถานะปุ่ม!");

    // หน่วงเวลาเล็กน้อยเพื่อป้องกันสัญญาณรบกวน (debounce)
    delay(50); 

    // เตรียมข้อมูลที่จะส่ง (Payload)
    // ถ้าปุ่มถูกกด (LOW) ส่ง "1", ถ้าปล่อย (HIGH) ส่ง "0"
    String dataToSend = (currentButtonState == LOW) ? "1" : "0";

    // ตรวจสอบว่ายังเชื่อมต่อ Wi-Fi อยู่หรือไม่ก่อนส่งข้อมูล
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(firebaseUrl); // เริ่มการเชื่อมต่อ HTTP ไปยัง Firebase URL
      
      // ระบุ Content-Type สำหรับการส่งข้อมูล (เป็น best practice)
      http.addHeader("Content-Type", "application/json");

      // ส่ง PUT request พร้อมกับข้อมูล (payload)
      Serial.printf("กำลังส่งข้อมูล: %s ไปยัง Firebase...\n", dataToSend.c_str());
      int httpCode = http.PUT(dataToSend);

      // ตรวจสอบผลลัพธ์
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.print("ส่งข้อมูลสำเร็จ! Firebase ตอบกลับ: ");
          Serial.println(payload);
        } else {
          Serial.printf("ส่งข้อมูลสำเร็จ แต่มีข้อผิดพลาดจาก Server, HTTP code: %d\n", httpCode);
        }
      } else {
        Serial.printf("PUT request ล้มเหลว, error: %s\n", http.errorToString(httpCode).c_str());
      }
      
      http.end(); // ปิดการเชื่อมต่อ

    } else {
      Serial.println("การเชื่อมต่อ Wi-Fi หลุด! ไม่สามารถส่งข้อมูลได้");
    }

    // อัปเดตสถานะล่าสุดของปุ่ม
    lastButtonState = currentButtonState;
    Serial.println("------------------------------------");
  }

  // ไม่ต้องมี delay หน่วงนานใน loop หลัก เพื่อให้สามารถตรวจจับการกดปุ่มได้ทันที
}