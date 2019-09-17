#ifndef TF_MINI_PLUS_H
#define TF_MINI_PLUS_H

#include <Arduino.h>
#include <Wire.h>

const uint8_t FRAME_START = 0x5A;

enum TFMINI_PLUS_COMMANDS {
    TFMINI_PLUS_GET_DATA = 0,
    TFMINI_PLUS_GET_VERSION = 1,
    TFMINI_PLUS_SYSTEM_RESET = 2,
    TFMINI_PLUS_SET_FRAME_RATE = 3,
    TFMINI_PLUS_TRIGGER_DETECTION = 4,
    TFMINI_PLUS_SET_OUTPUT_FORMAT = 5,
    TFMINI_PLUS_SET_BAUD_RATE = 6,
    TFMINI_PLUS_SWITCH_DATA_OUTPUT = 7,
    TFMINI_PLUS_SET_COMMUNICATION_INTERFACE = 0x0A,
    TFMINI_PLUS_SET_I2C_ADDRESS = 0x0B,
    TFMINI_PLUS_SET_IO_MODE = 0x3B,
    TFMINI_PLUS_RESTORE_FACTORY_SETTINGS = 0x10,
    TFMINI_PLUS_SAVE_SETTINGS = 0x11
};

typedef enum TFMINI_PLUS_IO_MODE {
    STANDARD = 0,

} tfminiplus_mode_t;

typedef enum TFMINI_PLUS_FRAMERATE {
    TFMINI_PLUS_FRAMERATE_0HZ = 0,
    TFMINI_PLUS_FRAMERATE_1HZ = 1,
    TFMINI_PLUS_FRAMERATE_2HZ = 2,
    TFMINI_PLUS_FRAMERATE_5HZ = 5,
    TFMINI_PLUS_FRAMERATE_10HZ = 10,
    TFMINI_PLUS_FRAMERATE_20HZ = 20,
    TFMINI_PLUS_FRAMERATE_25HZ = 25,
    TFMINI_PLUS_FRAMERATE_50HZ = 50,
    TFMINI_PLUS_FRAMERATE_100HZ = 100,
    TFMINI_PLUS_FRAMERATE_200HZ = 200,
    TFMINI_PLUS_FRAMERATE_250HZ = 250,
    TFMINI_PLUS_FRAMERATE_500HZ = 500,
    TFMINI_PLUS_FRAMERATE_1000HZ = 1000,
} tfminiplus_framerate_t;

typedef enum TFMINI_PLUS_BAUDRATE {
    TFMINI_PLUS_BAUDRATE_9600 = 9600,
    TFMINI_PLUS_BAUDRATE_19200 = 19200,
    TFMINI_PLUS_BAUDRATE_38400 = 38400,
    TFMINI_PLUS_BAUDRATE_57600 = 57600,
    TFMINI_PLUS_BAUDRATE_115200 = 115200
} tfminiplus_baudrate_t;

typedef union {
    uint8_t raw[6];
    struct {
        uint16_t distance;
        uint16_t strength;
        uint16_t temperature;
    };
} tfminiplus_data_t;

enum TFMINI_PLUS_OUTPUT_FORMAT { TFMINI_PLUS_OUTPUT_CM = 1, TFMINI_PLUS_OUTPUT_PIXHAWK = 2, TFMINI_PLUS_OUTPUT_MM = 6 };

enum TFMINI_PLUS_COMMUNICATION_MODE { TFMINI_PLUS_UART = 0, TFMINI_PLUS_I2C = 1 };

class TFminiPlus {
   public:
    bool begin(uint8_t address = 0x10);
    bool begin(Stream* stream);

    String get_version();
    bool set_framerate(tfminiplus_framerate_t framerate);

    bool set_i2c_address(uint8_t address);

   private:
    uint8_t _address;
    bool send(uint8_t* result);
};

#endif