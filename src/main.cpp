/**********************************************************************

【準備】
  I2Cで通信

RaspberrPy Pico - ST7735
3.3V - VCC
GND  - GND
GP20 - CS(チップセレクト)
GP19 - RESET(リセット信号入力)
GP18 - DC(データ/コマンド制御)
GP16 - SDA(シリアルデータ入力)
GP17 - SCL(シリアルクロック)
未接続 - BL(バックライト制御)

**********************************************************************/
//#include <Arduino.h>
#include "pico/stdlib.h"
#include <Adafruit_GFX.h> 
#include <Adafruit_ST7735.h>
#include <SPI.h>

//ピン番号設定
#define TFT_DC      28  // DC
#define TFT_CS      17  // CS
#define TFT_SCLK    18  // Clock
#define TFT_SDA    19  // SDA
#define TFT_RST     22  // Reset 


//色設定
#define RED ST77XX_RED
#define BLUE ST77XX_BLUE

void print_DisplayInfo(void);

//インスタンス生成
Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);


int main(){
  tft.initR(INITR_GREENTAB);                //Init ST7735S初期化(GREENTAB以外はバグ発生、部品種で変化)
  tft.fillScreen(ST77XX_BLACK);               //背景の塗りつぶし
  tft.setRotation(3);                         //画面回転
  print_DisplayInfo();
}

void print_DisplayInfo(){
  //ディスプレイの情報を取得し、表示
  tft.setTextSize(1);
  tft.printf("width*height\n");
  tft.printf("%d * %d\n",tft.width(),tft.height());
  tft.printf("Rotate\n");
  tft.printf("%d\n",tft.getRotation());
}