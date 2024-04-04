#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ST7735.h"
#include "Wire.h"
#include "MPU6050_light.h"

#define TFT_PIN_CS 10
#define TFT_PIN_DC 9
#define TFT_PIN_RST 8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);

MPU6050 mpu1(Wire);
MPU6050 mpu2(Wire);

int lastY1 = 0;
int lastY2 = 0;
int y1 = 0;
int y2 = 0;
int score1 = 0; // Player 1's score
int score2 = 0; // Player 2's score

// Ball properties
int ballX = 64, ballY = 80;
int ballSize = 4;
int ballVelX = 2, ballVelY = 2;

void setup() {
    Serial.begin(9600);
    Wire.begin();
    tft.initR(INITR_REDTAB);
    tft.setRotation(2);
    tft.fillScreen(ST7735_BLACK);

    mpu1.setAddress(0x68);
    mpu2.setAddress(0x69);
    mpu1.begin();
    mpu2.begin();
    mpu1.calcOffsets();
    mpu2.calcOffsets();

    // Display initial score
    displayScore();
}

void loop() {
    mpu1.update();
    mpu2.update();

    y1 = mapAngleToScreen(mpu1.getAngleX());
    y2 = mapAngleToScreen(mpu2.getAngleX());

    drawPaddles();
    lastY1 = y1;
    lastY2 = y2;

    tft.fillRect(ballX, ballY, ballSize, ballSize, ST7735_BLACK);
    updateBallPosition();
    tft.fillRect(ballX, ballY, ballSize, ballSize, ST7735_WHITE);

    delay(10);
}

int mapAngleToScreen(float angle) {
    return map(angle, -90, 90, 0, tft.height() - 20);
}

void drawRectangle(int x, int y, uint16_t color) {
    tft.fillRect(x, y, 4, 20, color);
}

void drawPaddles(){
  drawRectangle(0, lastY1, ST7735_BLACK);
  drawRectangle(124, lastY2, ST7735_BLACK);
  drawRectangle(0, y1, ST7735_WHITE);
  drawRectangle(124, y2, ST7735_WHITE);
}


void updateBallPosition() {
    ballX += ballVelX;
    ballY += ballVelY;

    // Bounce off top and bottom walls
    if (ballY <= 0 || ballY >= tft.height() - ballSize) {
        ballVelY = -ballVelY;
    }

    // Left paddle collision
    if (ballX <= 4) {
        if (ballY + ballSize >= lastY1 && ballY <= lastY1 + 20) {
            ballVelX = -ballVelX; // Bounce off the paddle
        } else if (ballX < 0) {
            // Player 2 scores
            score2++;
            newPoint();
            displayScore();
        }
    }

    // Right paddle collision
    else if (ballX >= tft.width() - 8) {
        if (ballY + ballSize >= lastY2 && ballY <= lastY2 + 20) {
            ballVelX = -ballVelX; // Bounce off the paddle
        } else if (ballX > tft.width() - ballSize) {
            // Player 1 scores
            score1++;
            newPoint();
            displayScore();
        }
    }
}


void resetBall() {
    ballX = tft.width() / 2;
    ballY = tft.height() / 2;
}

void newPoint() {
  resetBall();
  tft.fillRect(ballX, ballY, ballSize, ballSize, ST7735_WHITE);
  int timer = millis();
  while (millis()-timer < 1000){
    mpu1.update();
    mpu2.update();

    y1 = mapAngleToScreen(mpu1.getAngleX());
    y2 = mapAngleToScreen(mpu2.getAngleX());

    drawPaddles();
    lastY1 = y1;
    lastY2 = y2;
    drawPaddles();
  
  }
}
void displayScore() {
    tft.fillRect(0, 0, tft.width(), 10, ST7735_BLACK); // Clear the score area
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, 0);
    tft.print("P1: ");
    tft.print(score1);
    tft.setCursor(tft.width() / 2, 0);
    tft.print("P2: ");
    tft.print(score2);
}
