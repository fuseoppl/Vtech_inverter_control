Works with Dyno software from V-tech Dynamometers

additional shields for Arduino UNO:
 SKU:DFR0972 GP8302 0-25mA (current Loop D/A Converter)
 SKU:DFR1073 GP8413 2x 0-5V or 0-10V (voltage D/A Converter)

input data (Drivig cycles):
DYAAAABBBBCCCCDDDD2\r
 DY, header
 AAAA = actual speed, the input data is multiplied by 10 (for higher resolution), in hex
 eg.: 29.8 km/h * 10 = 298 to hex -> 012A
 BBBB = target speed, the input data is multiplied by 10 (for higher resolution), parameter from the Driving cycles mode, in hex
 CCCC = target speed seconds ahead, the input data is multiplied by 10 (for higher resolution), parameter from the Driving cycles mode, in hex
 DDDD = breaks control value in %, the input data is multiplied by 10 (for higher resolution), parameter from the Driving cycles mode, in hex
 2, semaphore (Driving cycles)
 \r end of line (carriage return)

 eg.: DY012A01120100023A2\r
 actual speed = 29.8 km/h
 target speed = 27.4 km/h
 target speed seconds ahead = 25.6 km/h
 breaks control = 57%

input data (other test):
DYAAAA1\r
 DY, header
 AAAA = actual speed, the input data is multiplied by 10 (for higher resolution), in hex
 eg.: 276.5 km/h * 10 = 2765 to hex -> 0ACD
 1, semaphore (other test)
 \r end of line (carriage return)

 eg.: DY0ACD1\r
 actual speed = 276.5 km/h
