// File: zone-detection.h
const static double BITWISE_FLIP = pow(2, 15);

// convert from serial input to position
int16_t convert_to_position(int serial_input, int default_value) {
  int16_t position = (uint16_t((serial_input << 8) | default_value ));
  if ((serial_input & 0x80) >> 7) {
    return position - BITWISE_FLIP;
  }
  return 0 - position;
} 

bool point_in_polygon(int min_x, int max_x, int min_y, int max_y, int16_t x, int16_t y) {
  return (min_x <= x && x <= max_x) && (min_y <= y && y <= max_y));
}
