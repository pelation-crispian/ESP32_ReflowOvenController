#include "Control.h"
#include <math.h>
#include "Ui.h"
#include "src/PID_v1/PID_v1.h"
#include "src/PID_AutoTune_v0/PID_AutoTune_v0.h"

static PID heaterPidController(&heaterInput, &heaterOutput, &heaterSetpoint, heaterPID.Kp, heaterPID.Ki, heaterPID.Kd, DIRECT);
static PID_ATune heaterPidTune(&heaterInput, &heaterOutput);

// Predictive inertia guard to reduce overshoot on high-thermal-mass ovens at low setpoints.
// Tunables are exposed in the PID menu and stored in NVS.

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
    casePrintState(ConstantTemp);
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

  // Match PID compute rate to temperature sampling interval.
  heaterPidController.SetSampleTime(READ_TEMP_INTERVAL_MS);
}

void controlUpdate(uint64_t time_ms) {
  if(!timer_control.repeat()) {
    return;
  }

  static State previousState= Idle;
  static uint64_t stateChangedTime_ms=time_ms;
  static bool wasPaused = false;
  static uint64_t pauseStart_ms = 0;
  static bool constantTempBeeped=false;
  boolean stateChanged=false;
  if (currentState != previousState) 
  {
    stateChangedTime_ms=time_ms;
    stateChanged = true;
    previousState = currentState;
    if (currentState != ConstantTemp) {
      constantTempBeeped = false;
    }
  }
  static float rampToSoakStartTemp;
  static float coolDownStartTemp;
  static float constTempRampedSetpoint;
  static uint64_t constTempRampTime_ms = 0;
  static uint64_t constTempStart_ms = 0;
  static bool integralSuppressed = false;
  bool paused = systemPaused;

  if (paused) {
    if (!wasPaused) {
      pauseStart_ms = time_ms;
      heaterPidController.SetMode(MANUAL);
    } else {
      uint64_t pauseDelta_ms = time_ms - pauseStart_ms;
      if (pauseDelta_ms > 0) {
        stateChangedTime_ms += pauseDelta_ms;
        cycleStartTime += pauseDelta_ms * 1000ULL;
        if (currentState == ConstantTemp) {
          constTempRampTime_ms += pauseDelta_ms;
        }
        pauseStart_ms = time_ms;
      }
    }
  } else if (wasPaused) {
    heaterPidController.SetTunings(heaterPID.Kp, heaterPID.Ki, heaterPID.Kd);
    heaterPidController.SetMode(AUTOMATIC);
  }
  wasPaused = paused;

  if (pidTuningsDirty) {
    float effectiveKi = heaterPID.Ki;
    if (currentState == ConstantTemp && integralSuppressed) {
      effectiveKi = 0.0f;
    }
    heaterPidController.SetTunings(heaterPID.Kp, effectiveKi, heaterPID.Kd);
    pidTuningsDirty = false;
  }

  heaterInput = aktSystemTemperature; 

  if (!paused) {
    switch (currentState) 
    {
    case RampToSoak:
      if (stateChanged) 
      {

        rampToSoakStartTemp=aktSystemTemperature;
        heaterSetpoint = rampToSoakStartTemp;

        heaterOutput = 0;
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
    case ConstantTemp:
      if (stateChanged) 
      {
        // Avoid resetting the PID integral by zeroing output before AUTO init.
        heaterPidController.SetMode(AUTOMATIC);
        heaterPidController.SetControllerDirection(DIRECT);
        const float error = (float)constantTempSetpoint - aktSystemTemperature;
        constTempStart_ms = time_ms;
        integralSuppressed = (time_ms - constTempStart_ms) < 10000 ||
                             ((integralEnableBandC > 0.0f) &&
                              (fabsf(error) > integralEnableBandC));
        const float effectiveKi = integralSuppressed ? 0.0f : heaterPID.Ki;
        heaterPidController.SetTunings(heaterPID.Kp, effectiveKi, heaterPID.Kd);
        heaterOutput = 0;
        constantTempBeeped = false;
        // Start slew-limited setpoint from the current temperature to avoid a large step.
        constTempRampedSetpoint = aktSystemTemperature;
        constTempRampTime_ms = time_ms;
      }

      // Slew the setpoint to reduce integral windup and overshoot on high-inertia ovens.
      if (constTempRampCps <= 0.0f) {
        heaterSetpoint = constantTempSetpoint;
      } else {
        if (constTempRampTime_ms == 0) {
          constTempRampTime_ms = time_ms;
          constTempRampedSetpoint = aktSystemTemperature;
        }
        const float dt_s = (time_ms - constTempRampTime_ms) / 1000.0f;
        if (dt_s > 0.0f) {
          const float target = (float)constantTempSetpoint;
          const float maxStep = constTempRampCps * dt_s;
          const float delta = target - constTempRampedSetpoint;
          if (delta > maxStep) {
            constTempRampedSetpoint += maxStep;
          } else if (delta < -maxStep) {
            constTempRampedSetpoint -= maxStep;
          } else {
            constTempRampedSetpoint = target;
          }
          constTempRampTime_ms = time_ms;
        }
        heaterSetpoint = constTempRampedSetpoint;
      }
      {
        const float error = heaterSetpoint - heaterInput;
        const bool suppressIntegral =
          (time_ms - constTempStart_ms) < 10000 ||
          ((integralEnableBandC > 0.0f) && (fabsf(error) > integralEnableBandC));
        if (suppressIntegral != integralSuppressed) {
          integralSuppressed = suppressIntegral;
          const float effectiveKi = integralSuppressed ? 0.0f : heaterPID.Ki;
          heaterPidController.SetTunings(heaterPID.Kp, effectiveKi, heaterPID.Kd);
        }
      }

      if (!constantTempBeeped && constantTempBeepMinutes > 0 &&
          (time_ms - stateChangedTime_ms) >= (uint32_t)constantTempBeepMinutes * 60 * 1000) {
        beepcount = 3;
        constantTempBeeped = true;
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
  }

  if (!paused) {
    // Only apply the guard in steady setpoint states so we don't distort ramp segments.
    static bool inertiaHold = false;
    const bool inertiaGuardEnabled = true;
    const bool inertiaGuardState =
      currentState == ConstantTemp ||
      currentState == Soak ||
      currentState == Peak;
    if (inertiaGuardEnabled &&
        inertiaGuardLeadSec > 0.0f &&
        inertiaGuardMaxSetpointC > 0.0f &&
        inertiaGuardState &&
        heaterSetpoint <= inertiaGuardMaxSetpointC) {
      const float predictedTemp =
        aktSystemTemperature + (aktSystemTemperatureRamp * inertiaGuardLeadSec);
      if (!inertiaHold) {
        // Enter hold when the predicted temperature crosses the setpoint while rising.
        if (aktSystemTemperatureRamp > inertiaGuardMinRiseCps &&
            predictedTemp >= heaterSetpoint) {
          inertiaHold = true;
        }
      } else {
        // Leave hold once we're safely below the setpoint prediction band.
        if (predictedTemp <= heaterSetpoint - inertiaGuardHysteresisC) {
          inertiaHold = false;
        }
        heaterOutput = 0;
      }
    } else {
      // If we were holding and the state changed, restore automatic control cleanly.
      if (inertiaHold) {
        const bool shouldBeAuto =
          currentState == RampToSoak ||
          currentState == Soak ||
          currentState == RampUp ||
          currentState == Peak ||
          currentState == ConstantTemp;
        if (shouldBeAuto) {
          heaterPidController.SetMode(AUTOMATIC);
        }
      }
      inertiaHold = false;
    }

    if (!inertiaHold && heaterPidController.Compute()) {
      pidOutP = heaterPidController.GetLastP();
      pidOutI = heaterPidController.GetLastI();
      pidOutD = heaterPidController.GetLastD();
    }
  } else {
    heaterOutput = 0;
    pidOutP = 0;
    pidOutI = 0;
    pidOutD = 0;
  }

  if (paused) {
    powerHeater = 0;
  } else if (
       currentState == RampToSoak ||
       currentState == Soak ||
       currentState == RampUp ||
       currentState == Peak ||
       currentState == ConstantTemp ||
       currentState == PreTune ||
       currentState == Tune          
     )
  {

    if (heaterSetpoint+100 < aktSystemTemperature) // if we're 100 degree cooler than setpoint, abort
    {
      reportError("Temperature is Way to HOT!!!!!"); 
    }
    // Zero-cross SSR uses burst control; linear duty maps to linear power.
    powerHeater = (uint16_t)heaterOutput;
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

  const bool runState =
    currentState == RampToSoak ||
    currentState == Soak ||
    currentState == RampUp ||
    currentState == Peak ||
    currentState == CoolDown ||
    currentState == ConstantTemp ||
    currentState == PreTune ||
    currentState == Tune;
  bool fanOn = runState && !paused;
  if (fanOverride == 1) fanOn = true;
  if (fanOverride == 0) fanOn = false;
  digitalWrite(FAN1, fanOn ? 1 : 0);
}
