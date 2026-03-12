#define PCA8574_ADDRESS1        0x38  // PCA8574 OUTPUT TTL control lines
#define PCA8574_ADDRESS2        0x39  // PCA8574 OUTPUT TTL control lines

// write to PCA8574 over I2C
void writeRegister(byte, int);
// read from PCA8574 over I2C
uint8_t readRegister(int);

