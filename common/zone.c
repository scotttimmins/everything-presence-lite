#include <vector>

const static int16_t MAX_INT_16 = pow(2, 15); // This causes a warning because of the type conversion however the value is correct
const static float RADIANS_TO_DEGREES = 180.0 / 3.14159265358979323846;

string get_mmwave_firmware(std::vector<uint8_t> response) {
  int firmware_major = response[13];
  int firmware_minor = response[12];
  char firmware_version_str[32];
  sprintf(firmware_version_str, "V%d.%02d", firmware_major, firmware_minor);
  return std::string(firmware_version_str);
}

int16_t check_bytes(uint8_t bytes1, uint8_t bytes2) {
  return (
    uint16_t(
      (bytes2 << 8) | bytes1
    )
  );
}

int16_t read_value(uint8_t bytes1, uint8_t bytes2) {
  int16_t value = check_bytes(bytes1, bytes2);
  if ((bytes2 & 0x80) >> 7) {
    return value - MAX_INT_16; 
  }
  return 0 - value;
}

std::tuple<int16_t, int16_t> adjust_position_by_angle(int16_t x, int16_t y, int16_t distance, float angle, float installation_angle) {
  if (installation_angle == 0) {
    return std::make_tuple(x, y);
  }

  float adjustment_angle = angle - installation_angle;
  int16_t new_x = distance * cos(adjustment_angle);
  int16_t new_y = distance * sin(adjustment_angle);
  return std::make_tuple(new_x, new_y);
}

bool is_in_range(int16_t min_value, int16_t max_value, int16_t value) {
  return (min_value <= value && value <= max_value);
}

bool is_in_zone(
  std::tuple<int16_t, int16_t> position,
  std::tuple<int16_t, int16_t, int16_t, int16_t> zone
) {
  auto [min_x, max_x, min_y, max_y] = zone;
  auto [x, y] = position;
  return is_in_range(min_x, max_x, x) && is_in_range(min_y, max_y, y);
}

std::tuple<
  std::tuple<
    int16_t, // x
    int16_t, // y
    int16_t, // speed
    int16_t, // distance_resolution
    int16_t, // angle
    int16_t, // distance
    bool // active
  >,
  std::tuple<
    bool, // targets_detected
    bool, // in occupancy_mask
    bool, // in zone 1
    bool, // in zone 2
    bool, // in zone 3
    bool // in zone 4
  >
> detect_target(
  int16_t max_distance,
  std::tuple<
    int16_t, // x
    int16_t, // y
    int16_t, // speed
    int16_t, // distance_resolution
    int16_t, // angle
    int16_t // distance
  > target,
  std::tuple<
    int16_t, // min x
    int16_t, // max x
    int16_t, // min y
    int16_t // max y
  > occupancy_mask,
  std::tuple<
    int16_t, // min x
    int16_t, // max x
    int16_t, // min y
    int16_t // max y
  > zone_1,
  std::tuple<
    int16_t, // min x
    int16_t, // max x
    int16_t, // min y
    int16_t // max y
  > zone_2,
  std::tuple<
    int16_t, // min x
    int16_t, // max x
    int16_t, // min y
    int16_t // max y
  > zone_3,
  std::tuple<
    int16_t, // min x
    int16_t, // max x
    int16_t, // min y
    int16_t // max y
  > zone_4
) {
  auto [x, y, speed, distance_resolution, angle, distance] = target;
  if (!is_in_range(0, max_distance, distance)) {
    return std::make_tuple(
      std::make_tuple(0, 0, 0, 0, 0, 0, false),
      std::make_tuple(false, false, false, false, false, false)
    );
  }

  std::tuple<int16_t, int16_t> position = adjust_position_by_angle(x, y, distance, angle, installation_angle);

  if (is_in_zone(position, occupancy_mask)) {
    return std::make_tuple(
      std::make_tuple(0, 0, 0, 0, 0, 0, false),
      std::make_tuple(true, true, false, false, false, false)
    );
  }

  auto [x, y] = position;
  return std::make_tuple(
    std::make_tuple(
      x,
      y,
      speed / 100,
      distance_resolution,
      distance,
      (angle * RADIANS_TO_DEGREES) - 90,
      true
    ),
    std::make_tuple(
      true,
      false,
      is_in_zone(position, zone_1),
      is_in_zone(position, zone_2),
      is_in_zone(position, zone_3),
      is_in_zone(position, zone_4)
    )
  );
}

std::tuple<
  int16_t, // x
  int16_t, // y
  int16_t, // speed
  int16_t, // distance_resolution
  int16_t, // angle
  int16_t // distance
> 
get_target(
  int16_t bytes1, 
  int16_t bytes2, 
  int16_t bytes3, 
  int16_t bytes4, 
  int16_t bytes5, 
  int16_t bytes6, 
  int16_t bytes7, 
  int16_t bytes8
) {
    int16_t x = read_value(bytes1, bytes2);
  int16_t y = read_value(bytes3, bytes4);
  return std::make_tuple(
    x,
    y,
    read_value(bytes5, bytes6),
    check_bytes(bytes7, bytes8),
    atan2(y, x),
    sqrt(x * x + y * y)
  );
}

std::tuple<int16_t, int16_t, int16_t, int16_t> make_zone(int16_t begin_x, int16_t end_x, int16_t begin_y, int16_t end_y) {
  return std::make_tuple(min(begin_x, end_x), max(begin_x, end_x), min(begin_y, end_y), max(begin_y, end_y));
}



