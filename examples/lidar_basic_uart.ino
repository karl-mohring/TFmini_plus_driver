#include <Arduino.h>
#include <TFmini_plus.h>
#include <SoftwareSerial.h>

///////////////////////////////////////////////////////////////////////////////

const int LIDAR_RX = D2;
const int LIDAR_TX = D1;
const long LIDAR_UART_BAUDRATE = 115200;

const long LOG_SERIAL_BAUD = 115200;

///////////////////////////////////////////////////////////////////////////////

SoftwareSerial soft_serial(LIDAR_RX, LIDAR_TX);
TFminiPlus lidar;

void setup()
{
  // Start up serial communications
  Serial.begin(LOG_SERIAL_BAUD);
  Serial.println("Started lidar test");

  // Start up UART communications with the lidar
  soft_serial.begin(LIDAR_UART_BAUDRATE);
  lidar.begin(&soft_serial);

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