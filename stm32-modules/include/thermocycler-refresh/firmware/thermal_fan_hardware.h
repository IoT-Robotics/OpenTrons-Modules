#ifndef THERMAL_FAN_HARDWARE_H__
#define THERMAL_FAN_HARDWARE_H__
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>

/**
 * @brief Initialize the fans on the board. This is a prerequisite
 * for controlling any fans.
 */
void thermal_fan_initialize(void);

/**
 * @brief Set the power level of the fans
 *
 * @param[in] power The power to set, as a percentage from 0.0F to 1.0F
 * inclusive. Values outside of this range will be clamped
 * @return True if the fan could be set to \p power, false if an error
 * occurred.
 */
bool thermal_fan_set_power(double power);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
#endif  /* THERMAL_FAN_HARDWARE_H__ */
