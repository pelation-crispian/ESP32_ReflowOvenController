// ----------------------------------------------------------------------------
// Reflow Oven Controller
// (c) 2019      Patrick Knöbel
// (c) 2014 Karl Pitrich <karl@pitrich.com>
// (c) 2012-2013 Ed Simmons
// ----------------------------------------------------------------------------

#include "Globals.h"
#include "Ui.h"
#include "Sensors.h"
#include "Control.h"
#include "root_html.h"

Neotimer timer_beep = Neotimer(500);
Neotimer timer_temp = Neotimer(READ_TEMP_INTERVAL_MS);
Neotimer timer_dsp_btmln = Neotimer(200);
Neotimer timer_display = Neotimer(1000);
Neotimer timer_control = Neotimer(100);
Neotimer timer_countupdown = Neotimer(200);

Button2 btn_startstop;
Button2 btn_up;
Button2 btn_down;
Button2 btn_left;
Button2 btn_right;
Button2 btn_stop;

bool countup = false;
bool countdown = false;

//Varables
const char * ver = "4.0";

//https://docs.google.com/spreadsheets/d/1syyYxHWjEHy5YJJYK1BmVZYog_yKRtXpa9yudWSkGAE/edit?usp=sharing
//(ASIN(A3/256*2-1)+PI()/2)/PI()*256
const uint8_t asinelookupTable[] {0,10,14,17,20,22,25,27,28,30,32,34,35,37,38,39,41,42,43,44,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,67,68,69,70,71,72,72,73,74,75,76,76,77,78,79,80,80,81,82,83,83,84,85,86,86,87,88,88,89,90,91,91,92,93,93,94,95,95,96,97,98,98,99,100,100,101,102,102,103,104,104,105,106,106,107,108,108,109,110,110,111,111,112,113,113,114,115,115,116,117,117,118,119,119,120,120,121,122,122,123,124,124,125,126,126,127,128,128,129,129,130,131,131,132,133,133,134,135,135,136,136,137,138,138,139,140,140,141,142,142,143,144,144,145,145,146,147,147,148,149,149,150,151,151,152,153,153,154,155,155,156,157,157,158,159,160,160,161,162,162,163,164,164,165,166,167,167,168,169,169,170,171,172,172,173,174,175,175,176,177,178,179,179,180,181,182,183,183,184,185,186,187,188,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,211,212,213,214,216,217,218,220,221,223,225,227,228,230,233,235,238,241,245};

SPIClass MYSPI(HSPI); 
Adafruit_ST7735 tft = Adafruit_ST7735(&MYSPI,LCD_CS, LCD_DC, LCD_RESET);
Thermocouple Temp_1;
Menu::Engine myMenue;
Preferences PREF;

WebServer server(80);
WebServer serverAction(8080);

volatile boolean globalError=false;

volatile uint16_t  powerHeater=0;

float aktSystemTemperature;
float aktSystemTemperatureRamp; //°C/s

int16_t tuningHeaterOutput=30;
int16_t tuningNoiseBand=1;
int16_t tuningOutputStep=10;
int16_t tuningLookbackSec=60;


int activeProfileId = 0;
Profile_t activeProfile; // the one and only instance

int16_t encAbsolute;

State currentState  = Idle;
uint64_t stateChangedTicks = 0;

// track menu item state to improve render preformance
LastItemState_t currentlyRenderedItems[MENUE_ITEMS_VISIBLE];

float heaterSetpoint;
float heaterInput;
float heaterOutput;

PID_t heaterPID;

bool menuUpdateRequest = true;
bool initialProcessDisplay = false;

uint64_t cycleStartTime=0;

uint8_t beepcount = 1;

void setup() {
  //Debug
  Serial.begin(115200);

  uiInit();

  //init timers
  timer_beep.start();
  timer_temp.start();
  timer_dsp_btmln.start();
  timer_display.start();
  timer_control.start();

  uiInitButtons();
  controlInit();
  sensorsInit();
  uiInitBuzzer();

  //init Wifi:
  WiFi.begin();  

  //init Webserver
  if (MDNS.begin("ReflowController")) {
      Serial.println("MDNS responder started");
  }
  server.on("/", []() {
    server.sendHeader("Cache-Control","no-cache");
    server.send(200, "text/html", ROOT_HTML);
  });
  server.on("/status", []() {
    server.sendHeader("Cache-Control","no-cache");
    char buffer[200];
    unsigned long time = (esp_timer_get_time()-cycleStartTime)/1000;
    snprintf(buffer,200,"{\"time\": %lu, \"temp\": %.2f, \"dt\": %.2f, \"setpoint\":  %.2f, \"power\": %.2f, \"state\": \"%s\"}",time,aktSystemTemperature,aktSystemTemperatureRamp,heaterSetpoint,heaterOutput*100/256,currentStateToString());
    server.send(200, "application/json", buffer);
  });
  serverAction.on("/start", []() {
    serverAction.sendHeader("Cache-Control","no-cache");
    serverAction.sendHeader("Access-Control-Allow-Origin","*");
    if(currentState == Settings)
    {
      //Start Revlow!
      myMenue.navigate(&miCycleStart);
      myMenue.invoke();
      serverAction.send(200, "text/plain", "OK");
    }
    else
    {
      serverAction.send(200, "text/plain", "ERROR");
    }
  });
  serverAction.on("/stop", []() {
    serverAction.sendHeader("Cache-Control","no-cache");
    serverAction.sendHeader("Access-Control-Allow-Origin","*");
    bool ok=false;
    if (currentState == Complete) 
    { 
      menuExit(Menu::actionDisplay); // reset to initial state
      myMenue.navigate(&miCycleStart);
      currentState = Settings;
      menuUpdateRequest = true;
      ok=true;
    }
    else if (currentState == CoolDown) 
    {
      currentState = Complete;
      ok=true;
    }
    else if (currentState > UIMenuEnd) 
    {
      currentState = CoolDown;
      ok=true;
    }
    if(ok){
      serverAction.send(200, "text/plain", "OK");      
    }
    else
    {
      serverAction.send(200, "text/plain", "ERROR");
    }
  });
  server.onNotFound([](){
    server.send(404, "text/plain", "404 :-(");
  });
  serverAction.onNotFound([](){
    serverAction.send(404, "text/plain", "404 :-(");
  });
  
  server.begin();  
  serverAction.begin();  

  //Preferences init
  PREF.begin("REFLOW");
  loadLastUsedProfile();
  loadPID();          
  
  uiShowSplash();

  delay(1000);

  menuExit(Menu::actionDisplay); // reset to initial state
  myMenue.navigate(&miCycleStart);
  currentState = Settings;
  menuUpdateRequest = true;
}

void loop()
{
  uint64_t time_ms = esp_timer_get_time()/1000;

  // Handle internet requests
  server.handleClient();  
  serverAction.handleClient();  

  uiHandleButtons();
  uiUpdateBeep();
  sensorsUpdate();
  uiUpdateBottomLine();
  uiUpdateMenu();
  uiUpdateProcessDisplay();
  controlUpdate(time_ms);
}
