#include <stdint.h>

void I2C_init(void);
void I2C_start(void);
void I2C_restart(void);
void I2C_stop(void);
void I2C_ack(void);
void I2C_nak(void);
uint8_t I2C_send(uint8_t i2c_data);
uint8_t I2C_read(void);
