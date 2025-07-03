# ch552_qt_py_libs
A set of small, high performance libraries for Adafruit's CH552 QT Py.

This is a set of fairly streamlined libraries designed specifically for the CH552 QT Py.  It should work with any CH552, but certain things are hardcoded for Adafruit's board layout.

## Libraries included

### I2C

This is a fairly complete, from scratch I2C driver, hardcoded to use the Stemma QT port on the CH552 QT Py.  The CH552 does not have hardware I2C, so this is a bitbanging driver.  This is mainly written in C, with delays written using inline assembly in macros.  I have not measured the timing, but at 16MHz, my math indicates it should run at around 333kbit/s.

#### Usage

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


### Delay

This is a pair of simple assembly delay functions, designed for extremely high precision delays.  These are tuned for 16MHz.  They will be proportionally shorter or longer if the clock speed is higher or lower respectively.  These are also size optimized.

It should be noted that if you use interrupts, and one of these delays is interrupted, it won't know that it was interrupted, and the time spent in the interupt service routine will be added to the delay.  If you really need ultra high precision timing, and you are using interrupts, you should disable interrupts before starting the time sensitive code and reenable them after.  These functions will not do that for you!

One final note: These delay 1 to 256 units.  If you need a delay between 256 and the next unit up, you'll have to use multiple delay calls.  Function call overhead may introduce some inaccuracy, though it will never be enough to matter for the millisecond variant.  For the microsecond variant, 16 cycles is one microsecond, and the overhead of four function calls _will_ add up to 16 or more cycles.  Depending on pre-call register juggling, four calls could easily take several microseconds.  If this is a problem for your particular use case, you might want to consider writing your timing dependent code in assembly, so that you can control this and precisely calculate function call overhead to subtract it from the delays.  (Or you could just do trial and error...  _I_ wouldn't recommend that though.)

#### Usage

`void delay_us(uint8_t us)`  
Delays for the given number of microseconds.  Note that due to loop optimization, an argument of 0 will give a 256 microsecond delay.  This function delays exactly 1/16th or 1/8th of a microsecond extra (1 or 2 cycles, depending on alignment).  (This includes return overhead but not call overhead.  Call overhead is generally unlikely to be more than one microsecond.)

`void delay_ms(uint8_t ms)`  
Delays for the given number of milliseconds.  Again, an argument of 0 is treated as 256.  This function delays an extra 4 or 5 cycles (depending on alignment), which is roughly 1/4th of a microsecond (exactly 1/4000th to 1/3200th of a millisecond) extra.  Function call overhead is also negligable.

#### Generic CH552 Use

This should work as-is for any CH552 set to run at 16MHz.  It may also run as-is for other CH55x variants and possibly even for other 8051 variants, as there is nothing specific to the CH55x line that it uses or requires.  For other clock speeds, the delay will be inversely proportional.  At 24MHz, a 120 microsecond delay should take 80 microseconds.  At 8MHz, a 120 microsecond delay should end up taking 240 microseconds.  These are so highly tuned to 16MHz that if you need something with similar precision for a different clock speed, you'll be better off writing new delay functions from scratch than trying to adjust the timing on these.  (Though the assembly code for these functions might be good reference for doing this!)

#### Future Work

This is _not_ a high priority, but a delay function with a resolution of 1 second might be a good idea.  That said, this is probably better done with the built in timer peripherals than with assembly level delays.
