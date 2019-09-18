#include <TFmini_plus.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Send a packet to the lidar.
 * Data is sent by either UART or I2C; whichever is actively in use.
 *
 * @param input: Array containing headers, payload, and checksum.
 * @param size: Number of bytes to send.
 * @return: True for successful transmission.
 */
bool TFminiPlus::send(uint8_t* input, uint8_t size) {
    bool result = false;
    if (_communications_mode == TFMINI_PLUS_UART) result = send_uart(input, size);
    if (_communications_mode == TFMINI_PLUS_I2C) result = send_i2c(input, size);

    return result;
}

/**
 * Send the packet over UART.
 * Both software and hardware implementations are allowed.
 * @param input: Data packet to send.
 * @param size: Number of bytes to send.
 * @return: True if the correct number of bytes were sent.
 */
bool TFminiPlus::send_uart(uint8_t* input, uint8_t size) {
    uint8_t bytes_sent = _stream->write(input, size) == size;
    _stream->flush();
    return bytes_sent == size;
}

/**
 * Send the packet over I2C.
 * Both software and hardware implementations are allowed.
 * @param input: Data packet to send.
 * @param size: Number of bytes to send.
 * @return: True if the correct number of bytes were sent.
 */
bool TFminiPlus::send_i2c(uint8_t* input, uint8_t size) {
    Wire.beginTransmission(_address);
    uint8_t bytes_sent = Wire.write(input, size);
    bool acknowledged = Wire.endTransmission();

    return (bytes_sent == size and acknowledged);
}

/**
 * Send a command packet to the lidar.
 * Command packets use the following structure:
 * [0] 0x5A - Command header
 * [1] Length of packet
 * [2] Command code
 * [3-n] Command arguments (Not all commands have arguments)
 * [n] Checksum
 *
 * @param command: 8-bit command to send; see TFMINI_PLUS_COMMANDS.
 * @param arguments: Container containing the command arguments in little-endian format.
 * @param size: Total number of bytes to send, including header and checksum.
 * @return: True if the transmission was successful.
 */
bool TFminiPlus::send_command(tfminiplus_command_t command, uint8_t* arguments, uint8_t size) {
    bool result;
    uint8_t packet[size];
    packet[0] = TFMINI_PLUS_FRAME_START;
    packet[1] = size;
    packet[2] = command;

    // Put arguments into the packet as-is, assuming the endianess has been handled elsewhere
    if (size > TFMINI_PLUS_MINIMUM_PACKET_SIZE) {
        for (size_t i = 0; i < size - TFMINI_PLUS_MINIMUM_PACKET_SIZE; i++) {
            packet[TFMINI_PLUS_MINIMUM_PACKET_SIZE - 1 + i] = arguments[i];
        }
    }

    // Slap on the checksum and run
    packet[size - 1] = calculate_checksum(packet, size - 1);
    result = send(packet, size);
    return result;
}

/**
 * Send a command packet to the lidar.
 * Only to be used with commands that do not take arguments.
 *
 * @param command: 8-bit command to send; see TFMINI_PLUS_COMMANDS.
 * @return: True if the transmission was successful.
 */
bool TFminiPlus::send_command(tfminiplus_command_t command) {
    return send_command(command, 0, TFMINI_PLUS_MINIMUM_PACKET_SIZE);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Get a packet from the lidar.
 * Data is received by either UART or I2c; whichever is actively in use.
 *
 * @param output: Container for received data, headers, and checksum.
 * @param size: Number of bytes expected to receive.
 * @return: True if the received length and checksum is valid
 */
bool TFminiPlus::receive(uint8_t* output, uint8_t size) {
    bool result = false;

    // Grab data from stream
    if (_communications_mode == TFMINI_PLUS_UART) result = receive_uart(output, size);
    if (_communications_mode == TFMINI_PLUS_I2C) result = receive_i2c(output, size);

    // Data is valid if the packet length matches the received length value and if the checksum matches
    if (result) {
        result = size == output[TFMINI_PLUS_PACKET_POS_LENGTH];
        result &= compare_checksum(output, size);
    }

    return result;
}

/**
 * Receive data from the UART.
 *
 * @param output: Container for data to read into.
 * @param size: Number of bytes expected to be read.
 * @return: True if the expected number of bytes was read.
 */
bool TFminiPlus::receive_uart(uint8_t* output, uint8_t size) {
    uint8_t bytes_read = _stream->readBytes(output, size);
    return bytes_read == size;
}

/**
 * Receive data from the I2C bus.
 *
 * @param output: Container for data to read into.
 * @param size: Number of bytes expected to be read.
 * @return: True if the expected number of bytes was read.
 */
bool TFminiPlus::receive_i2c(uint8_t* output, uint8_t size) {
    Wire.requestFrom(_address, size);

    size_t bytes_read;
    for (bytes_read = 0; (bytes_read < size) and Wire.available(); bytes_read++) {
        uint8_t c = Wire.read();
        output[bytes_read] = c;
    }

    return bytes_read == size;
}

/**
 * Get a data packet from UART.
 * Data packets can be received at any time, so a synchronised packet start is not guaranteed.
 * This function discards received data until a valid packet is found.
 * Only the first valid packet is read. More packets may be available in the stream's buffer.
 *
 * @param output: Container to read data into.
 * @param size: Number of bytes that are expected to be received
 * @return: True if the expected number of bytes were received.
 */
bool TFminiPlus::uart_receive_data(uint8_t* output, uint8_t size) {
    bool packet_start_found = false;

    // Discard data until a valid packet header is found (0x5959)
    while (_stream->available() and not packet_start_found) {
        if (_stream->read() == TFMINI_PLUS_RESPONSE_FRAME_HEADER and
            _stream->read() == TFMINI_PLUS_RESPONSE_FRAME_HEADER) {
            output[0] = TFMINI_PLUS_RESPONSE_FRAME_HEADER;
            output[1] = TFMINI_PLUS_RESPONSE_FRAME_HEADER;
            packet_start_found = true;
        }
    }

    // Read in the rest of the packet
    uint8_t bytes_read = _stream->readBytes(&output[2], size - 2) + 2;
    return bytes_read == size;
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Check if the calculate checksum of a data packet matches the sent checksum byte.
 *
 * @param data: Container of received packet.
 * @param size: Number of bytes contained in the packet, including headers and checksum.
 * @return: True if the checksums match.
 */
bool compare_checksum(uint8_t* data, uint8_t size) {
    uint8_t checksum = calculate_checksum(data, size - 1);
    bool checksums_match = checksum == data[size - 1];
    return checksums_match;
}

/**
 * Calculate the checksum for a container.
 *
 * @param data: Container with data to calculate checksum.
 * @param size: Number of bytes in container.
 * @return: Checksum of data in the container.
 */
uint8_t calculate_checksum(uint8_t* data, uint8_t size) {
    uint8_t checksum = 0;

    for (size_t i = 0; i < (size - 1); i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * Wait for the lidar to process a command.
 * This wait is only required when using the I2C bus.
 * This wait is also not required when requesting a data packet on either communication mode.
 */
void TFminiPlus::do_i2c_wait() {
    if (_communications_mode == TFMINI_PLUS_I2C) delay(100);
}

///////////////////////////////////////////////////////////////////////////////

bool TFminiPlus::begin(uint8_t address = 0x10) {
    _communications_mode = TFMINI_PLUS_I2C;
    _address = address & 0x7F;
}

bool TFminiPlus::begin(Stream* stream) {
    _communications_mode = TFMINI_PLUS_UART;
    _stream = stream;
}

bool TFminiPlus::set_i2c_address(uint8_t address) {
    bool result = false;

    send_command(TFMINI_PLUS_SET_I2C_ADDRESS, &address, TFMINI_PLUS_PACK_LENGTH_SET_I2C_ADDRESS);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_SET_I2C_ADDRESS];
    if (receive(response, sizeof(response)) and response[3] == address) {
        result = true;
        _address = address;
    }
    return result;
}

tfminiplus_version_t TFminiPlus::get_version() {
    tfminiplus_version_t version;

    send_command(TFMINI_PLUS_GET_VERSION);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_VERSION_RESPONSE];
    if (receive(response, sizeof(response)) and response[TFMINI_PLUS_PACKET_POS_COMMAND] == TFMINI_PLUS_GET_VERSION) {
        version.revision = response[3];
        version.minor = response[4];
        version.major = response[5];
    }

    return version;
}

bool TFminiPlus::set_framerate(tfminiplus_framerate_t framerate) {
    bool result = false;
    uint8_t argument[2];
    argument[0] = framerate | 0xFF;
    argument[1] = framerate >> 8;
    send_command(TFMINI_PLUS_SET_FRAME_RATE, argument, TFMINI_PLUS_PACK_LENGTH_SET_FRAME_RATE);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_SET_FRAME_RATE];
    if (receive(response, sizeof(response))) {
        if (argument[0] == response[3] and argument[1] == response[4]) result = true;
    }
    return result;
}

bool TFminiPlus::set_baudrate(tfminiplus_baudrate_t baudrate) {
    bool result = false;
    uint8_t argument[4];
    argument[0] = baudrate | 0xFF;
    argument[1] = baudrate >> 8;
    argument[2] = baudrate >> 16;
    argument[3] = baudrate >> 24;

    send_command(TFMINI_PLUS_SET_BAUD_RATE, argument, TFMINI_PLUS_PACK_LENGTH_SET_BAUD_RATE);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_SET_BAUD_RATE];
    if (receive(response, sizeof(response))) {
        uint32_t echoed_baudrate = response[3] + (response[4] << 8) + (response[5] << 16) + (response[6] << 24);
        if (argument[0] == response[3] and argument[1] == response[4] and argument[2] == response[5] and
            argument[3] == response[6])
            result = true;
    }
    return result;
}

bool TFminiPlus::set_output_format(tfminiplus_output_format_t format) {
    bool result = false;

    send_command(TFMINI_PLUS_SET_OUTPUT_FORMAT, (uint8_t*)&format, TFMINI_PLUS_PACK_LENGTH_SET_OUTPUT_FORMAT);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_SET_OUTPUT_FORMAT];
    if (receive(response, sizeof(response))) {
        if (format == response[3]) result = true;
    }

    return result;
}

bool TFminiPlus::read_manual_reading(tfminiplus_data_t& data) {
    send_command(TFMINI_PLUS_TRIGGER_DETECTION);
    do_i2c_wait();
    return read_data_response(data);
}

bool TFminiPlus::read_data(tfminiplus_data_t& data, bool in_mm_format) {
    if (_communications_mode == TFMINI_PLUS_I2C)
        send_command(TFMINI_PLUS_GET_DATA, (uint8_t*)&in_mm_format, TFMINI_PLUS_PACK_LENGTH_GET_DATA);
    return read_data_response(data);
}

bool TFminiPlus::read_data_response(tfminiplus_data_t& data) {
    bool result = false;

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_DATA_RESPONSE];
    if (_communications_mode == TFMINI_PLUS_UART) {
        result = uart_receive_data(response, sizeof(response));
    } else {
        result = receive(response, sizeof(response));
    }

    if (result) {
        data.distance = response[2] + (response[3] << 8);
        data.strength = response[4] + (response[5] << 8);
        data.temperature = response[6] + (response[7] << 8);
    }

    return result;
}

bool TFminiPlus::set_io_mode(tfminiplus_mode_t mode, uint16_t critical_distance = 0, uint16_t hysteresis = 0) {
    uint8_t arguments[5];
    arguments[0] = mode;
    arguments[1] = uint8_t(critical_distance);
    arguments[2] = critical_distance >> 8;
    arguments[3] = uint8_t(hysteresis);
    arguments[4] = hysteresis >> 8;

    send_command(TFMINI_PLUS_SET_IO_MODE, arguments, TFMINI_PLUS_PACK_LENGTH_SET_IO_MODE);
}

bool TFminiPlus::set_communication_interface(tfminiplus_communication_mode mode) {
    send_command(TFMINI_PLUS_SET_COMMUNICATION_INTERFACE, (uint8_t*)&mode,
                 TFMINI_PLUS_PACK_LENGTH_SET_COMMUNICATION_INTERFACE);
    if (save_settings()) {
        _communications_mode = mode;
    }
}

bool TFminiPlus::enable_output(bool output_enabled) {
    bool result = false;

    send_command(TFMINI_PLUS_ENABLE_DATA_OUTPUT, (uint8_t*)&output_enabled, TFMINI_PLUS_PACK_LENGTH_ENABLE_DATA_OUTPUT);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_ENABLE_DATA_OUTPUT];
    if (receive(response, sizeof(response))) {
        if (output_enabled == response[3]) result = true;
    }
    return result;
}

bool TFminiPlus::save_settings() {
    bool result = false;

    send_command(TFMINI_PLUS_SAVE_SETTINGS);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_SAVE_SETTINGS_RESPONSE];
    if (receive(response, sizeof(response))) {
        if (response[3] == 0) {
            result = true;
        }
    }
    return result;
}

bool TFminiPlus::reset_system() {
    bool result = false;
    send_command(TFMINI_PLUS_SYSTEM_RESET);
    do_i2c_wait();
    uint8_t response[TFMINI_PLUS_PACK_LENGTH_SYSTEM_RESET_RESPONSE];
    if (receive(response, sizeof(response))) {
        if (response[3] == 0) result = true;
    }
    return result;
}

bool TFminiPlus::factory_reset() {
    bool result = false;

    send_command(TFMINI_PLUS_RESTORE_FACTORY_SETTINGS);
    do_i2c_wait();

    uint8_t response[TFMINI_PLUS_PACK_LENGTH_RESTORE_FACTORY_SETTINGS_RESPONSE];
    if (receive(response, sizeof(response))) {
        if (response[3] == 0) result = true;
    }
    return result;
}