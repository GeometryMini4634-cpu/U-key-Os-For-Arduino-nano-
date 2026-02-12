#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int B_UP = 2, B_DOWN = 3, B_LEFT = 4, B_RIGHT = 5, B_OK = 6;
int mode = 0, menuIdx = 0;
const char* items[] = {"PAINT", "G-DASH", "SETTINGS", "SNAKE"};

unsigned long lastAction = 0;
int sleepTime = 15; 
int hiGD = 0, hiSnake = 0; 
float vccCache = 5.0;

// Глобалы игр
float pY = 40, v = 0;
int oX = 128, score = 0, gdMode = 0, oGapY = 32;
int8_t snX[25], snY[25]; // Массивы тела змейки
int8_t snLen = 3, snDir = 1, fX = 40, fY = 40;

// --- 1. СИСТЕМНЫЕ ФУНКЦИИ ---

float getVCC() {
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL | (ADCH << 8);
  return 1125300L / (float)result / 1000.0;
}

void startAnimation() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  for(int i = 0; i < 128; i += 8) {
    display.drawFastVLine(i, 0, 64, 1);
    display.display();
  }
  display.clearDisplay();
  display.setTextSize(2); display.setCursor(15, 15); display.print(F("U-key OS"));
  display.setTextSize(1); display.setCursor(30, 45); display.print(F("v1.0 Enjoy!"));
  display.display();
  delay(2000);
}

void drawStatusBar() {
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 1); display.print(vccCache, 1); display.print(F("V"));
  display.setCursor(95, 1); display.print(millis()/1000); display.print(F("s"));
  display.drawFastHLine(0, 9, 128, 1);
}

void drawMenu() {
  display.clearDisplay();
  drawStatusBar();
  for(int i = 0; i < 4; i++) {
    int y = 14 + (i * 12);
    if(menuIdx == i) { display.fillRect(0, y - 1, 128, 11, 1); display.setTextColor(0); }
    else { display.setTextColor(1); }
    display.setCursor(30, y); display.print(items[i]);
  }
  display.display();
}

void exitToMenu() {
  if (mode == 2 && score > hiGD) { hiGD = score; EEPROM.put(0, hiGD); }
  if (mode == 4 && (snLen-3) > hiSnake) { hiSnake = (snLen-3); EEPROM.put(4, hiSnake); }
  mode = 0; score = 0;
  display.invertDisplay(true); delay(100); display.invertDisplay(false);
  vccCache = getVCC();
  while(digitalRead(B_OK) == LOW);
  drawMenu();
}

// --- 2. ИГРЫ ---

void runGD() {
  display.clearDisplay();
  display.setTextColor(1);
  int nextMode = (score / 7) % 4;
  if (nextMode != gdMode) {
    display.drawFastVLine(oX, 0, 64, 1);
    if (oX < 15) { gdMode = nextMode; oX = 128; }
  }
  bool btn = (digitalRead(B_UP) == LOW);
  if (gdMode == 0) { v += 0.55; if (btn && pY >= 48) v = -5.0; } 
  else if (gdMode == 1) { v += (btn ? -0.4 : 0.4); v = constrain(v, -2.5, 2.5); }
  else if (gdMode == 2) { v += 0.4; if (btn) { v = -3.5; while(digitalRead(B_UP)==LOW); } }
  else { v = (btn ? -3.5 : 3.5); }
  pY += v; pY = constrain(pY, 0, 52);
  oX -= 5; if (oX < -15) { oX = 128; score++; oGapY = random(20, 44); }
  display.drawRect(15, pY, 8, 8, 1);
  if (gdMode == 0) {
    display.fillTriangle(oX, 63, oX+6, 48, oX+12, 63, 1);
    display.drawFastHLine(0, 63, 128, 1);
    if (oX < 23 && oX > 10 && pY > 40) exitToMenu();
  } else {
    int gap = (gdMode == 2) ? 17 : 13;
    display.fillRect(oX, 0, 8, oGapY - gap, 1); 
    display.fillRect(oX, oGapY + gap, 8, 64, 1);
    if (oX < 23 && oX > 10 && (pY < oGapY - gap || pY + 8 > oGapY + gap)) exitToMenu();
  }
  display.setCursor(0, 11); display.print(score); 
  display.setCursor(90, 11); display.print(F("HI:")); display.print(hiGD);
  display.display();
}

void runSnake() {
  display.clearDisplay();
  display.drawRect(0, 10, 128, 54, 1);
  if(digitalRead(B_UP)==LOW && snDir!=2) snDir=0;
  if(digitalRead(B_RIGHT)==LOW && snDir!=3) snDir=1;
  if(digitalRead(B_DOWN)==LOW && snDir!=0) snDir=2;
  if(digitalRead(B_LEFT)==LOW && snDir!=1) snDir=3;
  for(int i=snLen; i>0; i--) { snX[i]=snX[i-1]; snY[i]=snY[i-1]; }
  
  // ФИКС ДВИЖЕНИЯ ГОЛОВЫ
  if(snDir==0) snY[0]-=4; if(snDir==1) snX[0]+=4; if(snDir==2) snY[0]+=4; if(snDir==3) snX[0]-=4;
  
  if(abs(snX[0]-fX)<4 && abs(snY[0]-fY)<4) { if(snLen<24) snLen++; fX=(random(2,28)*4); fY=(random(4,13)*4); }
  display.fillRect(fX, fY, 3, 3, 1);
  for(int i=0; i<snLen; i++) display.fillRect(snX[i], snY[i], 4, 4, 1);
  if(snX[0]<=0 || snX[0]>=124 || snY[0]<=10 || snY[0]>=60) exitToMenu();
  
  display.setCursor(2, 11); display.print(F("APPLES:")); display.print(snLen-3);
  display.setCursor(85, 11); display.print(F("HI:")); display.print(hiSnake);
  display.display(); delay(90);
}

void runSettings() {
  display.clearDisplay(); display.setTextColor(1);
  display.setCursor(10, 15); display.print(F("SLEEP: ")); display.print(sleepTime); display.print(F("s"));
  display.setCursor(10, 35); display.print(F("D5 TO CHANGE"));
  if (digitalRead(B_RIGHT) == LOW) { sleepTime += 15; if(sleepTime > 60) sleepTime = 15; delay(200); }
  display.display();
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  for(int i = 2; i <= 6; i++) pinMode(i, INPUT_PULLUP);
  EEPROM.get(0, hiGD); if(hiGD < 0) hiGD = 0;
  EEPROM.get(4, hiSnake); if(hiSnake < 0) hiSnake = 0;
  vccCache = getVCC();
  startAnimation();
  drawMenu();
}

void loop() {
  if (sleepTime != 0 && millis() - lastAction > (unsigned long)sleepTime * 1000) { display.ssd1306_command(SSD1306_DISPLAYOFF); }
  for(int i=2; i<=6; i++) if(digitalRead(i)==LOW) { lastAction = millis(); display.ssd1306_command(SSD1306_DISPLAYON); }
  if (mode == 0) {
    if (digitalRead(B_UP) == LOW) { menuIdx--; if(menuIdx < 0) menuIdx = 3; drawMenu(); delay(180); }
    if (digitalRead(B_DOWN) == LOW) { menuIdx++; if(menuIdx > 3) menuIdx = 0; drawMenu(); delay(180); }
    if (digitalRead(B_OK) == LOW) { 
      mode = menuIdx + 1; pY = 40; oX = 128; 
      if(mode==4) { snLen=3; snDir=1; for(int i=0; i<25; i++) { snX[i]=60; snY[i]=32; } }
      display.clearDisplay(); while(digitalRead(B_OK) == LOW); 
    }
  } 
  else if (mode == 1) { static int px=64, py=32; if(digitalRead(B_UP)==LOW) py--; if(digitalRead(B_DOWN)==LOW) py++; if(digitalRead(B_LEFT)==LOW) px--; if(digitalRead(B_RIGHT)==LOW) px++; display.drawPixel(px, py, 1); display.display(); }
  else if (mode == 2) runGD();
  else if (mode == 3) runSettings();
  else if (mode == 4) runSnake();
  if (mode != 0 && digitalRead(B_OK) == LOW) {
    long t = millis();
    while(digitalRead(B_OK) == LOW) { if(millis() - t > 800) exitToMenu(); }
  }
}
