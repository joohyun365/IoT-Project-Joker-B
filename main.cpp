/*
  ESP32 HTTPClient Jokes API Example
  - Keypad Category Selector Mod -

  https://wokwi.com/projects/342032431249883731
  Copyright (C) 2022, Uri Shaked
  Modified by Gemini for user project
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";


// 사용 중인 핀: 2, 4, 5, 15, 18, 19, 23
// Hardware SPI (18, 19, 23)는 라이브러리가 자동 사용
#define TFT_DC 2
#define TFT_CS 15
#define TFT_RST 4 // 리셋 핀 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);


// --- 키패드 핀 설정 ---
const byte ROWS = 4; // 4 rows
const byte COLS = 4; // 4 columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Rows: 27, 26, 25, 33
// Cols: 32, 17, 16, 35
byte rowPins[ROWS] = {27, 26, 25, 33}; 
byte colPins[COLS] = {32, 17, 16, 35}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// URL을 동적으로 생성합니다.

// getJoke 함수가 'category'를 인자로 받도록 수정
String getJoke(String category) {
  HTTPClient http;
  http.useHTTP10(true);

  // 카테고리를 포함한 URL을 동적으로 생성
  String url = "https://v2.jokeapi.dev/joke/" + category;
  http.begin(url);
  
  http.GET();
  String result = http.getString();

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, result);

  // Test if parsing succeeds.
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "<error>";
  }

  String type = doc["type"].as<String>();
  String joke = doc["joke"].as<String>();
  String setup = doc["setup"].as<String>();
  String delivery = doc["delivery"].as<String>();
  http.end();
  return type.equals("single") ? joke : setup + "  " + delivery;
}

// nextJoke 함수도 'category'를 인자로 받도록 수정
void nextJoke(String category) {
  tft.setTextColor(ILI9341_WHITE);
  tft.println("\nLoading joke...");

  String joke = getJoke(category); // 인자 전달
  tft.setTextColor(ILI9341_GREEN);
  tft.println(joke);
}

void setup() {

  WiFi.begin(ssid, password, 6);

  tft.begin();
  tft.setRotation(1);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    tft.print(".");
  }

  tft.fillScreen(ILI9341_BLACK); // 화면 정리
  tft.setCursor(0, 0);
  tft.print("OK! IP=");
  tft.println(WiFi.localIP());

  // 자동 농담 로딩 대신 안내 메시지 출력
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println("\nPress Key (1-7)");
  tft.println("1:Misc 2:Programming");
  tft.println("3:Dark 4:Pun");
  tft.println("5:Spooky 6:Christmas");
  tft.println("7:Any");
}

void loop() {
  // 입력값 받기
  char key = keypad.getKey();

  if (key != NO_KEY) {
    String category = "";

    // 키 값에 따라 카테고리 매핑
    switch (key) {
      case '1':
        category = "Misc";
        break;
      case '2':
        category = "Programming";
        break;
      case '3':
        category = "Dark";
        break;
      case '4':
        category = "Pun";
        break;
      case '5':
        category = "Spooky";
        break;
      case '6':
        category = "Christmas";
        break;
      case '7':
        category = "Any";
        break;
      default:
        // 1-7 외의 키는 무시
        break;
    }

    // 유효한 카테고리(1-7)가 선택된 경우에만 실행
    if (category.length() > 0) {
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(0, 0);
      
      // 사용자 피드백: 어떤 카테고리를 선택했는지 표시
      tft.setTextColor(ILI9341_YELLOW);
      tft.setTextSize(2);
      tft.println("Selected: " + category);
      
      // 해당 카테고리로 새 농담 요청
      nextJoke(category);
    }
  }

  delay(100); // 루프 안정화를 위한 짧은 딜레이
}