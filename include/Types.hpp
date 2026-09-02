#pragma once

#define VINDRIKTNING_DATASET_SIZE 20

struct Vindriktning {
  uint16_t pm1_0 = 0;
  uint16_t pm2_5 = 0;
  uint16_t pm10 = 0;
};

#if defined(PIN_SCD4X_SCL) && defined(PIN_SCD4X_SDA)
#define CO2_SENSOR_ENABLED
#endif
