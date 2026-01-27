#include "Control.h"
#include "Ui.h"
#include "src/PID_v1/PID_v1.h"
#include "src/PID_AutoTune_v0/PID_AutoTune_v0.h"

static PID heaterPidController(&heaterInput, &heaterOutput, &heaterSetpoint, heaterPID.Kp, heaterPID.Ki, heaterPID.Kd, DIRECT);
static PID_ATune heaterPidTune(&heaterInput, &heaterOutput);

const char * currentStateToString()
{
  #define casePrintState(state) case state: return #state;
  switch (currentState) {
    casePrintState(RampToSoak);
    casePrintState(Soak);
    casePrintState(RampUp);
    casePrintState(Peak);
    casePrintState(CoolDown);
    casePrintState(Complete);
    casePrintState(PreTune);
    casePrintState(Tune);
    default: return "Idle";
  }
}

void controlInit() {
  //init switchPower
  pinMode(HEATER1, OUTPUT);
  pinMode(FAN1, OUTPUT);

  digitalWrite(FAN1, 0);

  ledc_timer_config_t ledc_timer;
  ledc_channel_config_t ledc_channel;
 
  ledc_timer.speed_mode   = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num    = LEDC_TIMER_0;
  ledc_timer.duty_resolution       = LEDC_TIMER_8_BIT;
  ledc_timer.freq_hz      = 5;
 
  ledc_channel.channel    = LEDC_CHANNEL_0;
  ledc_channel.gpio_num   = HEATER1;
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.timer_sel  = LEDC_TIMER_0;
  ledc_channel.duty       = 0;
 
  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel);
}

void controlUpdate(uint64_t time_ms) {
  if(!timer_control.repeat()) {
    return;
  }

  static State previousState= Idle;
  static uint64_t stateChangedTime_ms=time_ms;
  boolean stateChanged=false;
  if (currentState != previousState) 
  {
    stateChangedTime_ms=time_ms;
    stateChanged = true;
    previousState = currentState;
  }
  static float rampToSoakStartTemp;
  static float coolDownStartTemp;
  
  heaterInput = aktSystemTemperature; 

  switch (currentState) 
  {
    case RampToSoak:
      if (stateChanged) 
      {

        rampToSoakStartTemp=aktSystemTemperature;
        heaterSetpoint = rampToSoakStartTemp;

        heaterPidController.SetMode(AUTOMATIC);
        heaterPidController.SetControllerDirection(DIRECT);
        heaterPidController.SetTunings(heaterPID.Kp, heaterPID.Ki, heaterPID.Kd);
      }

      heaterSetpoint = rampToSoakStartTemp + (activeProfile.rampUpRate * (time_ms-stateChangedTime_ms)/1000.0);

      if (heaterSetpoint >= activeProfile.soakTemp) 
      {
        currentState = Soak;
      }
      break;

    case Soak:

      heaterSetpoint = activeProfile.soakTemp;

      if (time_ms - stateChangedTime_ms >= (uint32_t)activeProfile.soakDuration * 1000) 
      {
        currentState = RampUp;
      }
      break;

    case RampUp:

      heaterSetpoint = activeProfile.soakTemp + (activeProfile.rampUpRate * (time_ms-stateChangedTime_ms)/1000.0);

      if (heaterSetpoint >= activeProfile.peakTemp) 
      {
        currentState = Peak;
      }
      break;

    case Peak:

      heaterSetpoint = activeProfile.peakTemp;

      if (time_ms - stateChangedTime_ms >= (uint32_t)activeProfile.peakDuration * 1000) {
        currentState = CoolDown;
      }
      break;

    case CoolDown:
      if (stateChanged) {
        heaterPidController.SetMode(MANUAL);

        beepcount=3;  //Beep! We need the door open!!!

        //rampDown from the last setpoint
        coolDownStartTemp=heaterSetpoint;
      }

      heaterSetpoint = coolDownStartTemp - (activeProfile.rampDownRate * (time_ms - stateChangedTime_ms) / 1000.0);
      heaterOutput = 0;

      if (heaterSetpoint < IDLE_TEMP) {
          heaterSetpoint = IDLE_TEMP;
      }
      
      if (aktSystemTemperature < IDLE_TEMP && heaterSetpoint == IDLE_TEMP) {
        currentState = Complete;
        heaterPidController.SetMode(MANUAL);

        beepcount=1;  //Beep! We are done!!!

      }
      break;
    case PreTune:
      if(stateChanged)
      {
        heaterPidController.SetMode(MANUAL);
        heaterSetpoint = aktSystemTemperature;
          heaterOutput = 255*tuningHeaterOutput/100;
      }
      if(heaterSetpoint+tuningNoiseBand <aktSystemTemperature || heaterSetpoint-tuningNoiseBand >aktSystemTemperature ) {
        stateChangedTime_ms=time_ms;
        heaterSetpoint = aktSystemTemperature;
      }
      if (time_ms - stateChangedTime_ms >= tuningLookbackSec*2 * 1000) {
        currentState = Tune;
      }
      break;
    case Tune:
      if (stateChanged) 
      {
        heaterSetpoint = aktSystemTemperature;
        
        heaterPidTune.Cancel();
        heaterOutput = 255*tuningHeaterOutput/100;
        heaterPidTune.SetNoiseBand(tuningNoiseBand);
        heaterPidTune.SetOutputStep(255*tuningOutputStep/100);
        heaterPidTune.SetLookbackSec(tuningLookbackSec);
        heaterPidTune.SetControlType(CT_PID_NO_OVERSHOOT); //We want NO Overshoot :-)
      }

      int8_t val = heaterPidTune.Runtime();

      if (val != 0) 
      {
        currentState = CoolDown;
        heaterPID.Kp = heaterPidTune.GetKp();
        heaterPID.Ki = heaterPidTune.GetKi();
        heaterPID.Kd = heaterPidTune.GetKd();

        savePID();

        tft.setCursor(40, 40);
        tft.print("Kp: "); 
        printfloat2(heaterPID.Kp);
        tft.setCursor(40, 50);
        tft.print("Ki: "); 
        printfloat2(heaterPID.Ki);
        tft.setCursor(40, 60);
        tft.print("Kd: "); 
        printfloat2(heaterPID.Kd);
      }

      break;
  }

  heaterPidController.Compute();

  if (
       currentState == RampToSoak ||
       currentState == Soak ||
       currentState == RampUp ||
       currentState == Peak ||
       currentState == PreTune ||
       currentState == Tune          
     )
  {

    if (heaterSetpoint+100 < aktSystemTemperature) // if we're 100 degree cooler than setpoint, abort
    {
      reportError("Temperature is Way to HOT!!!!!"); 
    }
    //make it more linear!
    powerHeater = asinelookupTable[(uint8_t)heaterOutput]; 
  } 
  else if(currentState == Edit && myMenue.currentItem==&miManual)
  {
    powerHeater=(encAbsolute*255)/100;
    Serial.print("Manual Heating:");Serial.println(powerHeater);
  }
  else
  {
    powerHeater =0;
  }

  // Set SCR PWM to powerHeater
  //ledcWrite(SCR_PWM_CHANNEL, powerHeater);
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, powerHeater);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

  if (powerHeater > 0)
  {
    digitalWrite(FAN1, 1);
  }
  else
  {
    digitalWrite(FAN1, 0);
  }
}
