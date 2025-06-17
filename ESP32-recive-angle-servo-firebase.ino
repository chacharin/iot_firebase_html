/*
  โปรเจกต์: ESP32 ควบคุม Servo Motor ด้วยค่าจาก Firebase Realtime Database
  ผู้จัดทำ: ผู้เชี่ยวชาญการเขียนโค้ด ESP32 (สร้างตามโจทย์)
  
  หน้าที่:
  1. เชื่อมต่อ Wi-Fi
  2. อ่านค่าตัวเลข (0-180) จาก Firebase Realtime Database ที่ path "/test" ทุกๆ 1 วินาที
  3. แปลงข้อมูลที่ได้รับเป็นตัวเลข และตรวจสอบให้อยู่ในช่วง 0-180
  4. นำค่าที่ได้ไปควบคุมตำแหน่งของ Servo Motor ที่ต่ออยู่กับ Pin ที่กำหนด (GPIO 13)
  5. มอเตอร์จะขยับก็ต่อเมื่อค่าที่อ่านได้มีการเปลี่ยนแปลงจากค่าล่าสุดเท่านั้น
*/

// --- 1. ไลบรารีที่จำเป็น ---
#include <WiFi.h>          // สำหรับการเชื่อมต่อ Wi-Fi
#include <HTTPClient.h>    // สำหรับการส่ง request ไปยัง Firebase
#include <ESP32Servo.h>    // สำหรับการควบคุม Servo Motor

// --- 2. กรุณาแก้ไขข้อมูลในส่วนนี้ ---
const char* ssid = "DR_24U";      // ใส่ชื่อ Wi-Fi (SSID) ของคุณ
const char* password = "0823424267";  // ใส่รหัสผ่าน Wi-Fi ของคุณ

// URL ของ Firebase Realtime Database (ต้องลงท้ายด้วย .json และมี auth token)
String firebaseUrl = "https://chacharin-b70b7-default-rtdb.asia-southeast1.firebasedatabase.app/test.json?auth=9w1FYYpcfTRHGAm9grMcFTCisi1ZdK5B4E4Mzl9M";

// --- 3. การตั้งค่าฮาร์ดแวร์ ---
const int servoPin = 13;    // Pin ที่ต่อกับสายสัญญาณของ Servo Motor

// --- 4. ตัวแปรโกลบอล ---
Servo myServo;              // สร้างอ็อบเจกต์สำหรับควบคุม Servo
int lastAngle = -1;         // ตัวแปรสำหรับเก็บค่าองศาล่าสุด (ตั้งค่าเริ่มต้นเป็น -1 เพื่อให้ทำงานครั้งแรกเสมอ)


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

  // --- ตั้งค่า Servo Motor ---
  ESP32PWM::allocateTimer(0);       // จัดสรร Timer สำหรับ PWM (จำเป็นสำหรับ ESP32Servo)
  myServo.setPeriodHertz(50);       // ตั้งค่าความถี่มาตรฐานสำหรับ Servo (50Hz)
  // ทำการ attach Servo เข้ากับ Pin ที่กำหนด
  // ค่า min/max pulse (500/2500) เป็นค่าที่มักจะครอบคลุม 0-180 องศาได้ดี
  myServo.attach(servoPin, 500, 2500); 
  
  Serial.println("ตั้งค่า Servo Motor เรียบร้อย");
  Serial.println("------------------------------------");
}

void loop() {
  // ตรวจสอบสถานะการเชื่อมต่อ Wi-Fi ก่อนทำงาน
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(firebaseUrl); // เริ่มการเชื่อมต่อ HTTP

    int httpCode = http.GET(); // ส่ง GET request

    if (httpCode == HTTP_CODE_OK) { // ตรวจสอบว่า request สำเร็จหรือไม่
      String payload = http.getString();
      Serial.print("ข้อมูลดิบจาก Firebase: ");
      Serial.println(payload);

      // Firebase มักจะส่งข้อมูลตัวเลขกลับมาในรูปแบบ String ที่มีเครื่องหมายคำพูดครอบอยู่ (เช่น "90")
      // จึงต้องทำการลบเครื่องหมายคำพูดออกก่อนแปลงเป็นตัวเลข
      payload.replace("\"", "");

      int angleFromFirebase = payload.toInt(); // แปลง String เป็น Integer

      // ตรวจสอบและจำกัดค่าให้อยู่ในช่วง 0 - 180 องศาเสมอ เพื่อความปลอดภัย
      int targetAngle = constrain(angleFromFirebase, 0, 180);

      Serial.printf("ค่าที่ต้องการ: %d, ค่าที่จำกัดแล้ว: %d\n", angleFromFirebase, targetAngle);

      // ตรวจสอบว่าค่าองศาที่ได้มาใหม่ แตกต่างจากค่าล่าสุดหรือไม่
      // เพื่อป้องกันการสั่งงานเซอร์โวซ้ำๆ โดยไม่จำเป็น
      if (targetAngle != lastAngle) {
        Serial.printf("ค่ามีการเปลี่ยนแปลง! กำลังหมุน Servo ไปที่ %d องศา\n", targetAngle);
        myServo.write(targetAngle); // สั่งให้ Servo หมุนไปยังตำแหน่งเป้าหมาย
        lastAngle = targetAngle;    // อัปเดตค่าองศาล่าสุด
      } else {
        Serial.println("ค่าไม่เปลี่ยนแปลง ไม่ต้องสั่งงาน Servo");
      }

    } else {
      Serial.printf("GET request ล้มเหลว, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end(); // ปิดการเชื่อมต่อ HTTP

  } else {
    Serial.println("การเชื่อมต่อ Wi-Fi หลุด!");
  }
  
  Serial.println("------------------------------------");
  // หน่วงเวลา 1 วินาทีก่อนที่จะอ่านค่าจาก Firebase อีกครั้ง
  // สามารถปรับค่าได้ตามความต้องการ
  delay(1000); 
}