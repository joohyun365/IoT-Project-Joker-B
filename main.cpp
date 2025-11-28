/*
  Joke Machine Final Version
  Flow: Fetch Joke -> Send to Make(Get Translation) -> Show Both -> Wait for Rating -> Send Rating
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

String MAKE_WEBHOOK_URL = "https://hook.eu1.make.com/jw98d8tt874wvaevnt151jl2el7co05s"; 

#define TFT_DC 2
#define TFT_CS 15
#define TFT_RST 4
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const byte ROWS = 4; const byte COLS = 4;
char keys[ROWS][COLS] = { 
  {'1','2','3','A'}, 
  {'4','5','6','B'}, 
  {'7','8','9','C'}, 
  {'*','0','#','D'} 
};
byte rowPins[ROWS] = {27, 26, 25, 33}; 
byte colPins[COLS] = {32, 17, 16, 22};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int BUZZER_PIN = 14; 
enum MachineState { STATE_MENU, STATE_RATING };
MachineState currentState = STATE_MENU; 

// 데이터 보존용 변수
String currentJoke = "";
String currentCategory = "";
String currentKorean = "";

void beep(int f, int d, int p) { tone(BUZZER_PIN, f); delay(d); noTone(BUZZER_PIN); delay(p); }

// 영어 농담 가져오기 (JokeAPI)
String getJoke(String category) {
  HTTPClient http;
  http.useHTTP10(true);
  http.begin("https://v2.jokeapi.dev/joke/" + category);
  int code = http.GET();
  String result = (code > 0) ? http.getString() : "Error";
  http.end();
  
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, result);
  String type = doc["type"].as<String>();
  String joke = doc["joke"].as<String>();
  String setup = doc["setup"].as<String>();
  String delivery = doc["delivery"].as<String>();
  return type.equals("single") ? joke : setup + "\n" + delivery;
}

// Make.com과 통신하는 함수 (번역 요청/ 평점 전송. 재시도 로직)
String sendToMake(String category, String joke, int rating) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    return "WiFi Error";
  }

  String response = "Error";
  int maxRetries = 3; // 최대 3번 시도

  // 재시도 루프 시작
  for(int i = 0; i < maxRetries; i++) {
    
    // 시도할 때마다 클라이언트를 새로 생성 (메모리 초기화 효과)
    WiFiClientSecure *client = new WiFiClientSecure;
    
    if(client) {
      client->setInsecure(); 
      client->setHandshakeTimeout(30000); // 타임아웃 30초

      HTTPClient http;
      
      // Make.com 연결 시도
      Serial.printf("Attempt %d/%d connecting to Make.com...\n", i+1, maxRetries);
      
      if(http.begin(*client, MAKE_WEBHOOK_URL)) {
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<512> doc;
        doc["category"] = category;
        doc["joke"] = joke;
        doc["rating"] = rating; 

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        // POST 전송
        int httpCode = http.POST(jsonPayload);

        if (httpCode > 0) {
          response = http.getString();
          Serial.println("Success!");
          
          http.end();
          delete client;
          break; // 성공했으면 루프 탈출
        } else {
          Serial.printf("Failed, Error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      } else {
        Serial.println("Unable to connect");
      }

      // 실패 시 메모리 해제하고 잠시 대기
      delete client;
      delay(1000); // 1초 숨 고르기
      
    } else {
      Serial.println("Heap Full - Cannot create client");
      delay(500);
    }
  } // 루프 끝

  return response;
}

// 농담 로딩 및 화면 표시 (번역 먼저 실행)
void nextJoke(String category) {
  tft.fillScreen(ILI9341_BLACK); tft.setCursor(0,0);
  tft.println("Selected: " + category);
//   tft.fillScreen(ILI9341_BLACK);
//   tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Loading Joke...");
  
  // 영어 농담 가져오기
  currentCategory = category;
  currentJoke = getJoke(category);
  
  tft.println("Translating...");
  
  // Make.com에 평점 0으로 먼저 보내서 번역 받아오기
  currentKorean = sendToMake(currentCategory, currentJoke, 0);

  // 화면에 영어 + 한글 같이 출력
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  
  // 영어 출력 (초록색)
  tft.setTextColor(ILI9341_GREEN);
  tft.println(currentJoke);
  tft.println("");

  // 한글 출력 (흰색) - Wokwi LCD에선 깨질 수 있으나 Serial엔 잘 나옴. 지울 예정
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Kor: " + currentKorean); 
  Serial.println("\n[Korean Translation]: " + currentKorean); // 시리얼 모니터 확인용

  // 평점 요청
  tft.setTextColor(ILI9341_MAGENTA);
  tft.println("\n--------------------");
  tft.println("Rate this joke (1-5)");
  
  currentState = STATE_RATING; // 이제 사용자의 입력을 기다림
}

void showMenu() {
  currentState = STATE_MENU;
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println("\nSelect Category:\n1:Misc 2:Prog\n3:Dark 4:Pun\n5:Spooky 6:X-mas\n7:Any");
}

void showRatingThankYou(char score) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.printf("\nRating: %c/5\nSaved to Cloud!\n", score);
  
  delay(2000); 
  showMenu(); 
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6);
  
  tft.begin(); tft.setRotation(1);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(2);
  
  while (WiFi.status() != WL_CONNECTED) { delay(100); tft.print("."); }
  showMenu();
}

void loop() {
  char key = keypad.getKey();

  if (key != NO_KEY) {
    // [메뉴 상태] 카테고리 선택 -> 농담 로드 -> 번역 -> 화면 표시
    if (currentState == STATE_MENU) {
      String cat = "";
      switch (key) {
        case '1': cat = "Misc"; break; case '2': cat = "Programming"; break;
        case '3': cat = "Dark"; break; case '4': cat = "Pun"; break;
        case '5': cat = "Spooky"; break; case '6': cat = "Christmas"; break;
        case '7': cat = "Any"; break;
      }
      if (cat.length() > 0) {
        nextJoke(cat);
      }
    } 
    // [평점 상태] 내용을 다 보고 별점을 매김
    else if (currentState == STATE_RATING) {
      if (key >= '1' && key <= '5') {
        int ratingInt = key - '0';
        
        // 평점을 매기면 다시 Make.com에 전송 (진짜 저장용)
        Serial.println("Sending Rating...");
        sendToMake(currentCategory, currentJoke, ratingInt);
        
        showRatingThankYou(key);
      }
      else if (key == '*') {
        showMenu(); // 스킵
      }
    }
  }
  delay(10);
}
