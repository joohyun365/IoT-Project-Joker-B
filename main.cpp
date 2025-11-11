/*
  ESP32 HTTPClient Jokes API Example

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

// --- TFT 핀 설정 (기존 Joke Machine 핀 기준) ---
// 사용 중인 핀: 2, 4, 5, 15, 18, 19, 23 (SPI 핀 포함)
#define TFT_DC 2
#define TFT_CS 15
#define TFT_RST 4
// Hardware SPI (18, 19, 23)는 라이브러리가 자동으로 사용합니다.
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


// --- 기계의 현재 상태를 정의 ---
enum MachineState {
  STATE_MENU,   // 카테고리 선택 대기 중
  STATE_RATING  // 농담 평가 대기 중
};
MachineState currentState = STATE_MENU; // 시작은 메뉴 상태


// URL을 동적으로 생성합니다.


// getJoke 함수가 'category'를 인자로 받도록
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

// 메뉴 화면 출력 함수
void showMenu() {
  currentState = STATE_MENU; // 상태를 메뉴로 설정

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println("\nSelect Category:");
  tft.println("1:Misc 2:Prog");
  tft.println("3:Dark 4:Pun");
  tft.println("5:Spooky 6:X-mas");
  tft.println("7:Any");
}

// 평가 완료 화면 함수
void showRatingThankYou(char score) {
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.print("\n\nRating: ");
  tft.print(score);
  tft.println("/5");
  tft.println("Thank you!");

  // [TODO] 여기서 서보 모터 웃는 시늉 함수 호출
  // smileServoAction(score);
  // [TODO] 여기서 부저 효과음 함수 호출
  // playSound(score);

  delay(2000); 
  showMenu();  // 2초 보여주고 다시 메뉴로 돌아감
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

  // IP 주소 출력 후 바로 메뉴 호출
  tft.fillScreen(ILI9341_BLACK); // 화면 정리
  tft.setCursor(0, 0);
  tft.print("OK! IP=");
  tft.println(WiFi.localIP());

  delay(1000); // IP 주소 잠시 표시
  
  showMenu(); // 메뉴 화면 호출
}

void loop() {
  // 입력값 받기
  char key = keypad.getKey();

  if (key != NO_KEY) {

    // ------------------------------------------------
    // 상황 1: 메뉴 고르는 상태일 때 (STATE_MENU)
    // ------------------------------------------------
    if (currentState == STATE_MENU) {
      String category = "";
      switch (key) {
        case '1': category = "Misc"; break;
        case '2': category = "Programming"; break;
        case '3': category = "Dark"; break;
        case '4': category = "Pun"; break;
        case '5': category = "Spooky"; break;
        case '6': category = "Christmas"; break;
        case '7': category = "Any"; break;
        default: break; // 엉뚱한 키 무시
      }

      if (category.length() > 0) {
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(ILI9341_YELLOW);
        tft.setTextSize(2);
        tft.println("Selected: " + category);

        // 해당 카테고리로 새 농담 요청
        nextJoke(category);

        // 농담을 보여줬으니, 이제 상태를 '평가 대기'로 바꿈
        currentState = STATE_RATING;

        tft.setTextColor(ILI9341_MAGENTA);
        tft.println("\nRate this joke (1-5)");
        tft.println("or press '*' to skip");
      }
    }

    // ------------------------------------------------
    // 상황 2: 평가 기다리는 상태일 때 (STATE_RATING)
    // ------------------------------------------------
    else if (currentState == STATE_RATING) {
      if (key >= '1' && key <= '5') {
        // 1~5점 사이의 점수를 누름
        showRatingThankYou(key);
        // [TODO] 여기서 MQTT로 점수를 보내거나 저장하는 코드 추가
      }
      else if (key == '*') {
        // 평가 건너뛰기
        showMenu();
      }
      // 그 외의 키를 누르면 무시
    }
  }

  delay(100); // 루프 안정화를 위한 짧은 딜레이
}
