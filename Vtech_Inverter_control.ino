//Maciej Strzebonski
//fuse@op.pl
//ver.2.0
//works with Dyno software from V-tech Dynamometers
//SKU:DFR0972 GP8302 0-25mA (current Loop D/A Converter)
//SKU:DFR1073 GP8413 2x 0-5V or 0-10V (voltage D/A Converter)

#define GP8413_eOutputRange5V  //GP8413 DAC output range 0-5V instead 0-10V

//#define debugger //for debugging only!

#include "DFRobot_GP8XXX.h"
/**************************
---------------------------
| A2 | A1 | A0 | i2c_addr |
---------------------------
| 0  | 0  | 0  |   0x58   |
---------------------------
| 0  | 0  | 1  |   0x59   |
---------------------------
| 0  | 1  | 0  |   0x5A   |
---------------------------
| 0  | 1  | 1  |   0x5B   |
---------------------------
| 1  | 0  | 0  |   0x5C   |
---------------------------
| 1  | 0  | 1  |   0x5D   |
---------------------------
| 1  | 1  | 0  |   0x5E   |
---------------------------
| 1  | 1  | 1  |   0x5F   |
---------------------------
***************************/
DFRobot_GP8413 GP8413(/*deviceAddr=*/0x5F); //DAC 0-5V or 0-10V
DFRobot_GP8302 GP8302;                      //0-25mA

unsigned long actualTime      =    0;
unsigned long lastCommandTime =    0;
unsigned long waitCommandTime = 4000; //4 seconds after the last command, reset outputs
String inputString            = "";
String order                  = "";
String orderArgument          = "";
bool stringComplete           = false;
bool orderStart               = false;
bool orderEnd                 = false;
String orderStartCharacters   = "D";
String orderEndCharacters     = "Y";
String argumentCharacters     = "0123456789ABCDEF";
String speedkmhSemaphore      = "1"; //the input speed data is multiplied by 1
String speedkmhIdealSemaphore = "2"; //the input speed data is multiplied by 10 (for higher resolution)

float speedkmhMin             =     0; //min speed x1, the minimum speed that will be transferred to calculate the fan speed, 
//it can be assumed to be 0 and the minimum fan speed can be regulated by the minimum value of the converter controlling the inverter (currentLoopMin or DACOutMin)

float speedkmhMax             =   200; //max speed x1, full fan speed at this value
float speedkmh                =     0; //actual speed, the input data is multiplied by 1 or 10, depends on semaphore
float speedkmhIdeal           =     0; //target speed, the input data is multiplied by 10 (for higher resolution), parameter from the Driving Cycles mode
float breaksControl           =     0; //breaks control value in %, the input data is multiplied by 10 (for higher resolution)

bool     GP8302_is_working    = false;
uint16_t currentLoopMin       =  1146; //minimum value for slow fan speed (0 = 0mA, 655 = 4mA)
uint16_t currentLoopMax       =  3276; //maximum value (GP8302 output resolution is 12-bit, 2^12 - 1 = 4095 = 25mA, 3276 = 20mA)
uint16_t currentLoop          =     0;

bool     GP8413_is_working    = false;
uint16_t DACOutMin            =  6000; //minimum value for slow fan speed (0 = 0V)
uint16_t DACOutMax            = 32767; //maximum value (GP8413 output resolution is 15-bit, 2^15 - 1 = 32767 = 5 or 10V, depending on DAC configuration)
uint16_t DAC0Out              =     0;
uint16_t DAC1Out              =     0;

void setup(){
  Serial.begin(250000);

  if (GP8302.begin() != 0 ) {
    Serial.println("ERR:GP8302");
  }
  else {
    GP8302_is_working = true;
    GP8302.setDACOutElectricCurrent(currentLoopMin);
  }

  if (GP8413.begin() != 0 ) {
    Serial.println("ERR:GP8413");
  }
  else {
    GP8413_is_working = true;
  #if defined(GP8413_eOutputRange5V)  
    GP8413.setDACOutRange(GP8413.eOutputRange5V);  //0-5V
  #else  
    GP8413.setDACOutRange(GP8413.eOutputRange10V); //0-10V
  #endif
    GP8413.setDACOutVoltage(0, 0);
    GP8413.setDACOutVoltage(DACOutMin, 1);
  }

  lastCommandTime = millis();
}

void loop() {
  actualTime = millis();

//reset outputs if there is no input data for the time specified in the parameters
  if (actualTime > lastCommandTime + waitCommandTime) {
    if (GP8302_is_working) GP8302.setDACOutElectricCurrent(currentLoopMin);
    if (GP8413_is_working) {
      GP8413.setDACOutVoltage(        0, 0);
      GP8413.setDACOutVoltage(DACOutMin, 1);
    }
  }

  if (stringComplete) {
    noInterrupts();

    stringComplete = false;
    lastCommandTime = millis();

    if (order == "DY") { //DY, new protocol
      String _partString = "";
      String _semaphore = orderArgument.substring(orderArgument.length() - 1, orderArgument.length());

      if (_semaphore == speedkmhIdealSemaphore) { //full set of data from Driving cycles mode, speed x10
        _partString = orderArgument.substring(0, 4);
        speedkmh = strtol(_partString.c_str(), NULL, 16) / 10.0; //actual speed converter

        _partString = orderArgument.substring(4, 8);
        speedkmhIdeal = strtol(_partString.c_str(), NULL, 16) / 10.0; //target speed converter

        /*
        _partString = orderArgument.substring(8, 12);
        breaksControl = strtol(_partString.c_str(), NULL, 16) / 10.0; //breaks control value converter
        */

#if defined(debugger)
Serial.print(speedkmh);
Serial.print(";");
Serial.print(speedkmhIdeal);
Serial.print(";");
Serial.println(breaksControl);
Serial.flush();
#endif       
      }
      else if (_semaphore == speedkmhSemaphore) { //speed from other test modes for inverter control, speed x1
        _partString = orderArgument.substring(0, 4);
        speedkmh = strtol(_partString.c_str(), NULL, 16); //actual speed converter
        if (speedkmh < speedkmhMin) speedkmh = speedkmhMin;

#if defined(debugger)
Serial.println(speedkmh);
Serial.flush();
#endif       
      }

      if (_semaphore == speedkmhIdealSemaphore) { //full set of data
        if (GP8302_is_working) {
          //(Inverter, output is from currentLoopMin to currentLoopMax)
          currentLoop = (float(currentLoopMax - currentLoopMin) / (speedkmhMax - speedkmhMin)) * (speedkmh - speedkmhMin) + currentLoopMin;
          if (currentLoop < currentLoopMin) currentLoop = currentLoopMin;
          if (currentLoop > currentLoopMax) currentLoop = currentLoopMax;          
          GP8302.setDACOutElectricCurrent(currentLoop);
        }

        if (GP8413_is_working) {
          //Channel 0 (Throttle)
          //**************************
          //add throttle control code
          //**************************
          //speedkmhIdeal;
          //speedkmh;
          //breaksControl;
          //
          //DAC0Out = ;
          GP8413.setDACOutVoltage(DAC0Out, 0);

          //Channel 1 (Inverter, output is from DACOutMin to DACOutMax)
          DAC1Out = (float(DACOutMax - DACOutMin) / (speedkmhMax - speedkmhMin)) * (speedkmh - speedkmhMin) + DACOutMin;
          if (DAC1Out < DACOutMin) DAC1Out = DACOutMin;
          if (DAC1Out > DACOutMax) DAC1Out = DACOutMax;
          GP8413.setDACOutVoltage(DAC1Out, 1);
        } 
      }
      else if (_semaphore == speedkmhSemaphore) { //only actual speed
        if (GP8302_is_working) {
          currentLoop = (float(currentLoopMax - currentLoopMin) / (speedkmhMax - speedkmhMin)) * (speedkmh - speedkmhMin) + currentLoopMin;
          if (currentLoop < currentLoopMin) currentLoop = currentLoopMin;
          if (currentLoop > currentLoopMax) currentLoop = currentLoopMax;          
          GP8302.setDACOutElectricCurrent(currentLoop);
        }

        if (GP8413_is_working) {
          //Channel 0 (Throttle)
          GP8413.setDACOutVoltage(0, 0); //only actual speed, so only the inverter can be controlled!

          //Channel 1 (Inverter, output from DACOutMin to DACOutMax)
          DAC1Out = (float(DACOutMax - DACOutMin) / (speedkmhMax - speedkmhMin)) * (speedkmh - speedkmhMin) + DACOutMin;
          if (DAC1Out < DACOutMin) DAC1Out = DACOutMin;
          if (DAC1Out > DACOutMax) DAC1Out = DACOutMax;
          GP8413.setDACOutVoltage(DAC1Out, 1);
        }
      }
    }

    interrupts();
  }
}

void serialEvent() {
  while (Serial.available()) {
    if (!stringComplete && inputString.length() < 100) {
      char inChar = (char)Serial.read();

      if (inChar == '\n' || inChar == '\r') {
        inputString.trim();
        order          = inputString.substring(0,2);
        orderArgument  = inputString.substring(2);
        orderArgument.trim();
        inputString    = "";
        orderStart     = false;
        orderEnd       = false;
        stringComplete = true;
      }
      else if (!orderStart             && orderStartCharacters.indexOf(inChar) != -1) {
        orderStart     = true;
        inputString   += inChar;
      }
      else if (!orderEnd && orderStart && orderEndCharacters.indexOf(inChar)   != -1) {
        orderEnd       = true;
        inputString   += inChar;
      }
      else if (orderEnd                && argumentCharacters.indexOf(inChar)   != -1) {
        inputString   += inChar;
      }
    }
    else if (inputString.length() > 99) {
      inputString      = "";
      orderStart       = false;
      orderEnd         = false;
      stringComplete   = false;
    }
  }
}