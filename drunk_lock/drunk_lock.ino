// 음주하면 횃 잠금! — 2024 제9회 INU Maker 경진대회
// 출처: 팀 제작 기록(Notion). 횃불이 얼굴 그리기 부분은 원본 기록에서도 생략되어 있음.

#include <DigitShield.h>
#include <Stepper.h>
#include <EEPROM.h>
#include <TFT_22_ILI9225.h>

// TFT 핀 정의
#define TFT_RST 8
#define TFT_RS  9
#define TFT_CS  10  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 3   // 0 if wired to +5V directly

#define width 175 // LCD 가로 픽셀 수
#define height 219 // LCD 세로 픽셀 수

TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED);

const int stepsPerRevolution = 2048; // 모터의 기본 스텝 수

// 핀 연결
const int motorPin1 = 4;
const int motorPin2 = 5;
const int motorPin3 = 6;
const int motorPin4 = 7;

const int MQ3Pin = A0;    // MQ3 센서 핀 (알코올 감지)
const int ACL_limit = 400; // 알코올 임계값 (음주 판단 기준)

const int MQ3VPin = 0; // MQ3 전원 핀
const int motorVPin = 1; // 모터 전원 핀

Stepper myStepper(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4);
bool motorTriggered; // 모터 상태 플래그

void setup() {
  pinMode(MQ3Pin, INPUT); // MQ3 센서 핀 설정
  myStepper.setSpeed(10); // 모터 속도 설정 (RPM)
  Serial.begin(9600); // 시리얼 통신 초기화
  pinMode(MQ3VPin, OUTPUT); // MQ3 전원 핀 출력으로 설정
  pinMode(motorVPin, OUTPUT); // 모터 전원 핀 출력으로 설정
  digitalWrite(MQ3VPin, LOW); // MQ3 전원 끄기
  digitalWrite(motorVPin, HIGH); // 모터 전원 켜기
  DigitShield.begin(); // DigitShield 초기화
  tft.begin(); // TFT 초기화

  // EEPROM에서 이전 상태를 읽음
  motorTriggered = EEPROM.read(0);

  // 항상 초기화 시 잠금 상태로 만듦
  if (motorTriggered) {
    Serial.println("이전에 잠금이 해제된 상태입니다. 다시 잠깁니다.");
    myStepper.step(-(stepsPerRevolution / 2)); // 잠금 동작
    motorTriggered = false; // 상태 초기화
    EEPROM.write(0, motorTriggered); // EEPROM에 저장
  } else {
    Serial.println("이미 잠겨 있는 상태입니다.");
  }
  image(1); // 초기 화면 출력
  digitalWrite(motorVPin, LOW); // 모터 전원 끄기
}

// 센서 값을 안정화하기 위한 함수
int stabilizeSensor(int pin, int samples = 10) {
  int total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin); // 센서 값 읽기
    delay(500); // 안정화 대기
  }
  return total / samples; // 평균값 반환
}

// 굵은 직선 그리는 함수
void drawBoldline(int x1, int y1, int x2, int y2, int bold) {
    int y;
    for (int i = x1; i < x2; i++) {
      y = round((float((y2 - y1)) / float((x2 - x1))) * float((i - x1))) + y1; // 선의 y좌표 계산
      tft.fillCircle(i, y, bold, COLOR_BLUE); // 굵은 선 그리기
    }
}

// 횃불이 얼굴 출력 함수
void image(int what) {
  if (what == 0) {
    tft.fillRectangle(0, 0, width, height, COLOR_RED); // 정색 얼굴 배경
    // 얼굴 그리기...
  } else if (what == 1) {
    tft.fillRectangle(0, 0, width, height, COLOR_WHITE); // 윙크 얼굴 배경
    // 얼굴 그리기...
  } else {
    tft.fillRectangle(0, 0, width, height, COLOR_WHITE); // 기본 얼굴 배경
    // 얼굴 그리기...
  }
}

void loop() {
  // Serial 모니터로부터 입력 받기
  image(1); // 윙크 얼굴 출력
  Serial.println("음주 측정을 하려면 '1'을 입력하세요.");
  while (Serial.available() == 0) {
    // 대기
  }

  String input = Serial.readStringUntil('\n'); // 줄바꿈 문자까지 읽기
  input.trim(); // 앞뒤 공백 및 줄바꿈 제거
  int inputValue = input.toInt(); // 문자열을 정수로 변환

  if (inputValue == 1) {
    digitalWrite(MQ3VPin, HIGH); // MQ3 전원 켜기
    Serial.println("알코올 농도 측정 시작...");

    // 알코올 값 측정 (MQ3)
    int MQ3value = stabilizeSensor(MQ3Pin); // 센서 값 평균화
    Serial.print("MQ3 (Alcohol) Value: ");
    Serial.println(MQ3value);
    digitalWrite(MQ3VPin, LOW); // MQ3 전원 끄기
    // 알코올 임계값 비교
    if (MQ3value > ACL_limit) {
      image(0); // 음주 상태 얼굴 출력
      Serial.println("음주 상태 감지: 킥보드 잠금 유지!");
      delay(3000);
    } else {
      image(2); // 음주 아님 얼굴 출력
      digitalWrite(motorVPin, HIGH); // 모터 전원 켜기
      Serial.println("음주 아님: 킥보드 잠금 해제!");
      myStepper.step(stepsPerRevolution / 2); // 모터 반대 방향 회전 (잠금 해제)
      motorTriggered = true; // 상태 업데이트
      EEPROM.write(0, motorTriggered); // 상태 저장
      Serial.println("모터가 해제되었습니다. 시스템 종료.");
      digitalWrite(motorVPin, LOW); // 모터 전원 끄기
      while (true); // 시스템 멈춤
    }
  }

  delay(100); // 측정 간격
}
