#include <Arduino.h>
#include <TFmini_plus.h>
#include <Wire.h>

///////////////////////////////////////////////////////////////////////////////

const int LIDAR_SDA = D2;
const int LIDAR_SCL = D1;
const int LIDAR_I2C_ADDRESS = 0x10;

const long LOG_SERIAL_BAUD = 115200;

///////////////////////////////////////////////////////////////////////////////

TFminiPlus lidar;

void setup()
{
  // Start up serial communications
  Serial.begin(LOG_SERIAL_BAUD);
  Serial.println("Started lidar test");

  // Start up I2C communications with the lidar
  Wire.begin(LIDAR_SDA, LIDAR_SCL);
  lidar.begin(LIDAR_I2C_ADDRESS);

  // Set lidar options (saving is important)
  lidar.set_framerate(TFMINI_PLUS_FRAMERATE_10HZ);
  lidar.set_output_format(TFMINI_PLUS_OUTPUT_CM);
  lidar.enable_output(true);
  lidar.save_settings();
}

void loop()
{
  // Grab a reading from the lidar
  tfminiplus_data_t data;
  bool result = lidar.read_data(data, true);

  // Display reading
  String output = "Distance: " + String(data.distance) + " cm\t";
  output += "Strength: " + String(data.strength) + "\t";
  output += "Temperature: " + String(data.temperature) + " °C";
  Serial.println(output);

  delay(200);
}
