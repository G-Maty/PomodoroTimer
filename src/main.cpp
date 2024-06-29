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
#define UP1BT         11
#define UP10BT        12
#define DOWN1BT       13
#define DOWN10BT      9
#define SETBT         8
#define NULBT       7


//色設定
#define RED ST77XX_RED
#define BLUE ST77XX_BLUE
#define DISP_MAX_HIGHT 128
#define DISP_MAX_WIDH 160


//グローバル変数
/*
0:タイマー停止中
1:workTimeカウントダウン中
2:breakTimeカウントダウン中
3:workTimeセット中
4:breakTimeセット中
*/
char TimerStatus = 0;
bool BreakFlg = false; //breakTimeカウントダウン有無
unsigned long MaxWorkTime = 3600; //デフォルト1h
unsigned long MaxBreakTime = 300; //デフォルト5m
unsigned long WorkTime = 0;  //作業用カウンタ
unsigned long BreakTime = 0;  //作業用カウンタ


//プロトタイプ宣言
void print_DisplayInfo(void);
void print_Header(void);
void print_Time(void);
bool Timer(struct repeating_timer *t);
void set_WorkTime(void);
void set_BreakTime(void);

//割り込み初期設定
struct repeating_timer st_timer;

//インスタンス生成
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

int main(){
  unsigned long pre_w_cnt = MaxWorkTime;
  unsigned long pre_b_cnt = MaxBreakTime;
  unsigned char isStart;  //スタートボタン状態格納
  unsigned char isSet;    //セットボタン状態格納
  unsigned char setbt_tmp;  //マルチスキャン対策
  unsigned char strtbt_tmp; //マルチスキャン対策
  //unsigned char timer_st = 0;

  //ピン初期設定
  pinMode(UP1BT,INPUT_PULLDOWN);
  pinMode(DOWN1BT,INPUT_PULLDOWN);
  pinMode(UP10BT,INPUT_PULLDOWN);
  pinMode(DOWN10BT,INPUT_PULLDOWN);
  pinMode(SETBT,INPUT_PULLDOWN);
  pinMode(STBT,INPUT_PULLDOWN);
  pinMode(LED_BUILTIN,OUTPUT);

  //ディスプレイ初期化
  tft.initR(INITR_GREENTAB);                //Init ST7735S初期化(GREENTAB以外はバグ発生、部品種で変化)
  tft.fillScreen(ST77XX_BLACK);               //背景の塗りつぶし
  tft.setRotation(3);                         //画面回転

  //タイマー初期化
  WorkTime = MaxWorkTime;
  BreakTime = MaxBreakTime;

  //print_DisplayInfo();
  /* タイマーの初期化(割込み間隔はusで指定) */
  add_repeating_timer_us(1000000, Timer, NULL, &st_timer);


  while(1){
    print_Header();
    print_Time();

    /* 状態遷移 */
    //キー読み取り(マルチスキャン対策)
    isSet = digitalRead(SETBT);
    isStart = digitalRead(STBT);
    sleep_ms(10);
    setbt_tmp = digitalRead(SETBT);
    strtbt_tmp = digitalRead(STBT);
    //停止状態 -> セット状態
    if(isSet != setbt_tmp){
      if((isSet && (TimerStatus == 0)) || (isSet && (TimerStatus == 4)))   {
        TimerStatus = 3;
      }else if(isSet && (TimerStatus == 3)){
        TimerStatus = 4;
      }
    }
    //セット状態 -> 停止状態
    if(isStart != strtbt_tmp){
      if((isStart && (TimerStatus == 3)) || (isStart && (TimerStatus == 4))){
        TimerStatus = 0;
      }
    }
    //停止状態 <-> カウント開始状態
    if(isStart != strtbt_tmp){
      if(isStart && (TimerStatus == 0)){ //停止 -> カウント
        if(!BreakFlg){
          TimerStatus = 1;
        }else{
          TimerStatus = 2;
        }
      }else if(isStart && ((TimerStatus == 1) || (TimerStatus == 2))){
        TimerStatus = 0;  //カウント停止
      }
    }

    /* 各状態中の処理 */
    if(TimerStatus == 3){
      set_WorkTime();
    }else if(TimerStatus == 4){
      set_BreakTime();
    }

    //tft.setTextSize(2);
    //tft.setCursor(10,80);
    //tft.printf("Chill Mode");
    
  }
}

/* 
割り込み処理
戻り値の型は必ずbool型 
*/
bool Timer(struct repeating_timer *t) {
  if(!(TimerStatus == 1) && !(TimerStatus == 2)){ //1,2以外
    return true;
  }
  
  if(TimerStatus == 1){
    WorkTime--;
  }else if(TimerStatus == 2){
    BreakTime--;
  }

  //各タイマー消化時の自動遷移
  if((WorkTime == 0) && (TimerStatus == 1)){ //work消化
    WorkTime = MaxWorkTime;
    BreakFlg = true;
    TimerStatus = 2;
  }
  if((BreakTime == 0) && (TimerStatus == 2)){ //break消化
    BreakTime = MaxBreakTime;
    BreakFlg = false;
    TimerStatus = 1;
  }
  return true;
}

void print_Header(){
    tft.setTextSize(1.8);
    tft.setCursor(40,10);
    tft.println("PomodoroTimer");
    tft.drawLine(0,20,160,20,ST77XX_RED);
}

void print_Time(){
  static unsigned long pre_w_cnt = 0;
  static unsigned long pre_b_cnt = 0;

  //時間が変化した時表示更新
  if((pre_w_cnt != WorkTime) || (pre_b_cnt != BreakTime)){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(1.8);
    pre_w_cnt = WorkTime;
    pre_b_cnt = BreakTime;
    tft.setCursor(10,40);
    int hour = WorkTime / 3600;
    int minute = (WorkTime % 3600) / 60;
    int seconds = (WorkTime % 3600) % 60;
    tft.printf("Work: %dh %dm %ds\n",hour, minute, seconds);
    tft.setCursor(10,60);
    hour = BreakTime / 3600;
    minute = (BreakTime % 3600) / 60;
    seconds = (BreakTime % 3600) % 60;
    tft.printf("Break: %dh %dm %ds\n",hour, minute, seconds);
    tft.printf("Status:%d\n", TimerStatus);
  }
}

void set_WorkTime(){
  if(digitalRead(UP1BT)){
    WorkTime += 60;
    if(WorkTime > 36000){ //max10時間まで
      WorkTime = 36000;
    }
    WorkTime = (WorkTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }else if(digitalRead(UP10BT)){
    WorkTime += 600;
    if(WorkTime > 36000){ //max10時間まで
      WorkTime = 36000;
    }
    WorkTime = (WorkTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }else if(digitalRead(DOWN1BT)){
    WorkTime -= 60;
    if(WorkTime > 36000){ //max10時間まで
      WorkTime = 0;
    }
    WorkTime = (WorkTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }else if(digitalRead(DOWN10BT)){
    WorkTime -= 600;
    if(WorkTime > 36000){
      WorkTime = 0;
    }
    WorkTime = (WorkTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }
  MaxWorkTime = WorkTime;    //設定値を保存
  BreakFlg = false;   //設定後はworkからカウントダウン
}

void set_BreakTime(){
  if(digitalRead(UP1BT)){
    BreakTime += 60;
    if(BreakTime > 36000){ //max10時間まで
      BreakTime = 36000;
    }
    BreakTime = (BreakTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }else if(digitalRead(UP10BT)){
    BreakTime += 600;
    if(BreakTime > 36000){ //max10時間まで
      BreakTime = 36000;
    }
    BreakTime = (BreakTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }else if(digitalRead(DOWN1BT)){
    BreakTime -= 60;
    if(BreakTime > 36000){ //max10時間まで
      BreakTime = 0;
    }
    BreakTime = (BreakTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }else if(digitalRead(DOWN10BT)){
    BreakTime -= 600;
    if(BreakTime > 36000){
      BreakTime = 0;
    }
    BreakTime = (BreakTime / 60) * 60; //設定した時に必ず秒数は0 
    sleep_ms(100);
  }
  MaxBreakTime = BreakTime;    //設定値を保存 
  BreakFlg = false;   //設定後はworkからカウントダウン
}

void print_DisplayInfo(){
  //ディスプレイの情報を取得し、表示
  tft.setTextSize(1);
  tft.printf("width*height\n");
  tft.printf("%d * %d\n",tft.width(),tft.height());
  tft.printf("Rotate\n");
  tft.printf("%d\n",tft.getRotation());
}