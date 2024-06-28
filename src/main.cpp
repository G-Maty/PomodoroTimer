/**********************************************************************

【準備】
  SPIで通信,配線に注意!

RaspberrPy Pico - ST7735
3.3V - VCC
GND  - GND
GP20 - CS(チップセレクト)
GP19 - RESET(リセット信号入力)
GP18 - DC(データ/コマンド制御)
GP16 - MOSI(SDA×) (シリアルデータ入力)
GP17 - CLK(SCL×)  (シリアルクロック)
未接続 - BL(バックライト制御)

Pico(Rp2040)動作クロック:125MHz
最大5時間(18000秒)まで計測可能

**********************************************************************/
//#include <Arduino.h>
#include "pico/stdlib.h"
#include <Adafruit_GFX.h> 
#include <Adafruit_ST7735.h>

//ピン番号設定
#define TFT_DC      28  // DC
#define TFT_CS      17  // CS
#define TFT_SCLK    18  // Clock
#define TFT_SDA    19  // SDA
#define TFT_RST     22  // Reset 
#define STBT         10
#define UPBT         11
#define DOWNBT       12


//色設定
#define RED ST77XX_RED
#define BLUE ST77XX_BLUE
#define DISP_MAX_HIGHT 128
#define DISP_MAX_WIDH 160


//グローバル変数
unsigned long TimerCount = 0;
/*
0:タイマー停止中,1:カウントダウン中,2:カウントセット中
*/
char TimerStatus = 0;
unsigned long WorkTime = 3600; //デフォルト1h
unsigned long BreakTime = 300; //デフォルト5m

//プロトタイプ宣言
void print_DisplayInfo(void);
void print_Header(void);
bool Timer(struct repeating_timer *t);

//割り込み初期設定
struct repeating_timer st_timer;

//インスタンス生成
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

int main(){
  unsigned long preCount = TimerCount;
  unsigned char isStart;
  unsigned char timer_st = 0;


  tft.initR(INITR_GREENTAB);                //Init ST7735S初期化(GREENTAB以外はバグ発生、部品種で変化)
  tft.fillScreen(ST77XX_BLACK);               //背景の塗りつぶし
  tft.setRotation(3);                         //画面回転
  //ピン初期設定
  pinMode(UPBT,INPUT_PULLDOWN);
  pinMode(DOWNBT,INPUT_PULLDOWN);
  pinMode(STBT,INPUT_PULLDOWN);
  pinMode(LED_BUILTIN,OUTPUT);

  //print_DisplayInfo();
  /* タイマーの初期化(割込み間隔はusで指定) */
  add_repeating_timer_us(1000000, Timer, NULL, &st_timer);


  while(1){
    print_Header();

    if(digitalRead(UPBT)){
      if(((WorkTime % 3600) % 60) % 60 == 0){ //ひっかかりを作る
        sleep_ms(200);
      }
      WorkTime += 10;
    }else if(digitalRead(DOWNBT)){
      if(((WorkTime % 3600) % 60) % 60 == 0){ //ひっかかりを作る
        sleep_ms(200);
      }
      if(WorkTime < 0) break;
      WorkTime -= 10;
    }

    tft.setTextSize(2);
    tft.setCursor(10,80);
    tft.printf("Chill Mode");

    //時間が変化した時表示更新
    if(preCount != WorkTime){
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(1.8);
      preCount = WorkTime;
      tft.setCursor(10,40);
      int hour = WorkTime / 3600;
      int minute = (WorkTime % 3600) / 60;
      int seconds = (WorkTime % 3600) % 60;
      tft.printf("%dh %dm %ds\n",hour, minute, seconds);
    }

    //マルチスキャン対策
    isStart = digitalRead(STBT);
    sleep_ms(10);
    char tmp = digitalRead(STBT);
    if(isStart != tmp){
      if(isStart && (TimerStatus == 0)){
        TimerStatus = 1;
      }else if(isStart && (TimerStatus == 1)){
        TimerStatus = 0;  //カウント停止
      }
    }
  }
}

/* 
割り込み処理
戻り値の型は必ずbool型 
*/
bool Timer(struct repeating_timer *t) {
  if(TimerCount < 0 || (TimerStatus == 0)){
    return true;
  }
  WorkTime--;
  return true;
}

void print_Header(){
    tft.setTextSize(1.8);
    tft.setCursor(40,10);
    tft.println("PomodoroTimer");
    tft.drawLine(0,20,160,20,ST77XX_RED);
}

void print_DisplayInfo(){
  //ディスプレイの情報を取得し、表示
  tft.setTextSize(1);
  tft.printf("width*height\n");
  tft.printf("%d * %d\n",tft.width(),tft.height());
  tft.printf("Rotate\n");
  tft.printf("%d\n",tft.getRotation());
}