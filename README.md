# TFmini_plus_driver

Arduino-based I2C/UART driver for the Benewake TFmini Plus

## Features

| Feature               | Working                                     |
| --------------------- | ------------------------------------------- |
| UART receiving        | yes                                         |
| UART sending          | limited to Hardware UART (see Known Issues) |
| I2C receving          | yes                                         |
| I2C sending           | yes                                         |
| Accuracy calculation  | untested, but yes                           |
| Checksum verification | yes                                         |
| IO mode(s)            | Not supported                               |

## Known Issues

-   SoftwareSerial does not appear to be able to write correctly to the lidar UART at 115200 baud. Data is received correctly, but changing and saving options do not appear to work (at least all the time). Try using a hardware UART port to change the baudrate to a lower setting if you need to use a software-implemented serial UART port. Remember to save your settings for changes to take effect.

-   I2C mode does not appear to work with mm units. The datasheet mentions changing the output units, but the output seemed to be stuck in cm in testing.

-   Temperature readings appear to sit up around 60°C, which may or may not be accurate for the lidar's internals. The case gets warm, so the reading might be real, but do not expect to use the lidar temperature to gauge ambient temperature levels.
