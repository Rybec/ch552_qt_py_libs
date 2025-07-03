# ch552_qt_py_libs
A set of small, high performance libraries for Adafruit's CH552 QT Py.

This is a set of fairly streamlined libraries designed specifically for the CH552 QT Py.  It should work with any CH552, but certain things are hardcoded for Adafruit's board layout.

## Libraries included

### I2C

#### Usage

This is a fairly complete, from scratch I2C driver, hardcoded to use the Stemma QT port on the CH552 QT Py.  The CH552 does not have hardware I2C, so this is a bitbanging driver.  This is mainly written in C, with delays written using inline assembly in macros.  I have not measured the timing, but at 16MHz, my math indicates it should run at around 333kbit/s.

`void I2C_init(void)`  
Sets up the pins for the I2C port.  This should be run once, before any other I2C function.

`void I2C_start(void)`  
Starts an I2C transaction.

`void I2C_restart(void)`  
Restarts a currently open I2C transaction.

`void I2C_stop(void)`  
Stops a currently open I2C transaction.

`void I2C_ack(void)`  
Sends I2C ack bit.  

`void I2C_nak(void)`  
Sends I2C nak bit.

`uint8_t I2C_send(uint8_t i2c_data)`  
Sends one byte of data to the I2C bus and returns ack.

`uint8_t I2C_read(void)`  
Reads one byte of data from the I2C bus and returns it.


#### Generic CH552 Use

This is hardcoded to use the Stemma QT port on Adafruit's CH552 QT Py.  If you want to use it with another CH552 system, and you are using the same pins (port 3 pins 3 and 4, for SCL and SDA respectively), this should work for you as is.  If you need to change the pins used, you will find the pin/port variable declarations after the delay macro definitions.  Note that these two pins are assigned to two sets of variables.  The ones named after port/pin number are only used in `I2C_init()`.  Everywhere else `SCL` and `SDA` are used instead.  You will also need to change the `MOD_OC` and `DIR_PU` port addresses, and you'll need to change their setup in `I2C_init()` to point to the right pins.  And lastly, these are also used in `I2C_read()` and `I2C_send()`, to change the direct of SDA.  Those will have to be adjusted to fit the new pin mapping as well.  Note that if performance is not a huge issue, ch55xduino has a more generic I2C driver for Arduino that is much easier to remap.

Also note that this is designed for the highest clock speed supported by the CH552 QT Py.  Due to the board voltage, this is 16MHz.  If you use a lower clock rate, I2C speed will be decreased proportionally.  If your CH552 board is supplying 5V, and you set the clock speed to 24MHz, I2C speed will be 50% faster, which is likely to exceed specifications for 400kbit/s peripherals.  You can adjust for this by modifying the delay macros, I can tell you that this is a huge pain.  This is another case where you might want to use ch55xduino, unless you need the speed so badly that you are willing to spend hours doing the math and assembly juggling to manually retune the delays.  (If you are running slower than 16MHz, and you need fast I2C, retuning is the only option.  This applies to the CH552 QT Py as well.)


#### Future Work

This currently sends one byte per function call.  This almost certainly reduces the bit rate significantly below the estimated maximum.  It needs a function that can send an entire buffer of arbitrary length all at once, to eliminate unnecessary function call overhead when sending large amounts of data.  It might also be worth adding a buffered multi-byte read function at some point as well.
