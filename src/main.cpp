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
#define TFT_DC      26  // DC
#define TFT_CS      17  // CS
#define TFT_SCLK    18  // Clock
#define TFT_SDA    19  // SDA(MOSI)
#define TFT_RST     27  // Reset 
#define STBT         11
#define UP1BT         8
#define UP10BT        7
#define DOWN1BT       9
#define DOWN10BT      10
#define SETBT         12
#define NULBT       13
#define BZR         21


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
unsigned long MaxWorkTime = 5; //デフォルト1h = 3600
unsigned long MaxBreakTime = 5; //デフォルト5m = 300
unsigned long WorkTime = 0;  //作業用カウンタ
unsigned long BreakTime = 0;  //作業用カウンタ


//プロトタイプ宣言
void print_DisplayInfo(void);
void print_Header(void);
void print_Time(void);
bool Timer(struct repeating_timer *t);
void print_Setting(void);
void print_Footer(void);
void set_WorkTime(void);
void set_BreakTime(void);
void call_Time(void);

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
  pinMode(BZR, OUTPUT);
  pinMode(NULBT,INPUT_PULLDOWN);

  //ディスプレイ初期化
  tft.initR(INITR_GREENTAB);                //Init ST7735S初期化(GREENTAB以外はバグ発生、部品種で変化)
  tft.fillScreen(ST7735_BLACK);               //背景の塗りつぶし
  tft.setRotation(3);                         //画面回転

  //タイマー初期化
  WorkTime = MaxWorkTime;
  BreakTime = MaxBreakTime;

  //print_DisplayInfo();
  /* タイマーの初期化(割込み間隔はusで指定) */
  add_repeating_timer_us(1000000, Timer, NULL, &st_timer);


  while(1){
    /* 出力 */
    if(TimerStatus == 3 || TimerStatus == 4){
      print_Setting();
    }else{
      print_Time();
    }
    print_Footer();

    /* 状態遷移 */
    //キー読み取り(マルチスキャン対策)
    isSet = digitalRead(SETBT);
    isStart = digitalRead(STBT);
    sleep_ms(100);
    setbt_tmp = digitalRead(SETBT);
    strtbt_tmp = digitalRead(STBT);
    //停止状態 -> セット状態
    if(isSet != setbt_tmp){
      if(isSet && (TimerStatus == 0)){
        tft.fillScreen(ST7735_BLACK); //画面を一度クリーン
        TimerStatus = 3;
      }else if(isSet && (TimerStatus == 4)){
        TimerStatus = 3;
      }else if(isSet && (TimerStatus == 3)){
        TimerStatus = 4;
      }
    }
    //セット状態 -> 停止状態
    if(isStart != strtbt_tmp){
      if((isStart && (TimerStatus == 3)) || (isStart && (TimerStatus == 4))){
        tft.fillScreen(ST7735_BLACK); //画面を一度クリーン
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
    call_Time();
    TimerStatus = 2;
  }
  if((BreakTime == 0) && (TimerStatus == 2)){ //break消化
    BreakTime = MaxBreakTime;
    BreakFlg = false;
    call_Time();
    TimerStatus = 1;
  }
  return true;
}

void print_Header(){
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1.8);
    tft.setCursor(40,10);
    tft.println("PomodoroTimer");
    tft.drawLine(0,20,160,20,ST77XX_RED);
}

/* 時間表示 */
/* メインで常にコール */
void print_Time(){
  static unsigned long pre_w_cnt = 0;
  static unsigned long pre_b_cnt = 0;

  print_Header();
  //時間が変化した時表示更新
  if((pre_w_cnt != WorkTime) || (pre_b_cnt != BreakTime)){
    //tft.fillScreen(ST7735_BLACK); fillScreenで画面を更新すると、画面ちらつきの元になる
    pre_w_cnt = WorkTime;
    pre_b_cnt = BreakTime;

    int hour;
    int minute;
    int seconds;
    hour = BreakTime / 3600;
    minute = (BreakTime % 3600) / 60;
    seconds = (BreakTime % 3600) % 60;
    /*
    setTextColor(文字色、背景色)
    背景色を設定することで、表示を変えた時に元の文字に被さって表示がおかしくなるのを防ぐ
    */
    if(TimerStatus != 2){ //worktime表示
      hour = WorkTime / 3600;
      minute = (WorkTime % 3600) / 60;
      seconds = (WorkTime % 3600) % 60;
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setCursor(15,50); 
      tft.setTextSize(2);
      tft.printf("%2dh %2dm %2ds\n",hour, minute, seconds);
    }else{
      hour = BreakTime / 3600;
      minute = (BreakTime % 3600) / 60;
      seconds = (BreakTime % 3600) % 60;
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setCursor(15,50); 
      tft.setTextSize(2);
      tft.printf("%2dh %2dm %2ds\n",hour, minute, seconds);
    }
    /*
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10,30);
    tft.printf("Work\n");
    tft.setTextSize(2);
    tft.printf("%2dh %2dm %2ds\n",hour, minute, seconds);
    */
    /*
    tft.setCursor(10,60);
    tft.setTextSize(1);
    tft.printf("Chill\n");
    tft.setTextSize(2);
    tft.printf("%2dh %2dm %2ds\n",hour, minute, seconds);
    */
  }
  //カーソルの消去
  tft.fillTriangle(140,75, 150,70, 150,80, ST7735_BLACK); //オフ
  tft.fillTriangle(140,45, 150,40, 150,50, ST7735_BLACK); //オフ

}

void print_Setting(){

  print_Header();
  int hour = WorkTime / 3600;
  int minute = (WorkTime % 3600) / 60;
  int seconds = (WorkTime % 3600) % 60;
  /*
  setTextColor(文字色、背景色)
  背景色を設定することで、表示を変えた時に元の文字に被さって表示がおかしくなるのを防ぐ
  */
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10,30);
  tft.printf("Work\n");
  tft.setTextSize(2);
  tft.printf("%2dh %2dm %2ds\n",hour, minute, seconds);
  hour = BreakTime / 3600;
  minute = (BreakTime % 3600) / 60;
  seconds = (BreakTime % 3600) % 60;
  tft.setCursor(10,60);
  tft.setTextSize(1);
  tft.printf("Chill\n");
  tft.setTextSize(2);
  tft.printf("%2dh %2dm %2ds\n",hour, minute, seconds);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  if(TimerStatus == 3){
    //セット中のカーソル
    tft.fillTriangle(140,45, 150,40, 150,50, ST7735_GREEN); //オン
    tft.fillTriangle(140,75, 150,70, 150,80, ST7735_BLACK); //オフ
  }else if(TimerStatus == 4){
    tft.fillTriangle(140,75, 150,70, 150,80, ST7735_GREEN); //オン
    tft.fillTriangle(140,45, 150,40, 150,50, ST7735_BLACK); //オフ
  }
  //フッター
  tft.drawRect(10,90,140,30, ST7735_RED);
  tft.setCursor(45,100);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.println("Setting Mode");
}

void print_Footer(){
  tft.drawRect(10,90,140,30, ST7735_RED);
}

void call_Time(){
  //tone(pin, freq, duration)
  tone(BZR, 500, 50);
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