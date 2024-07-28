#include "adc-sensors.h"
#include "app_conf.h"
#include "adc.h"
#include "shtc3.h"
#include "i2c.h"
#include "sht4x_i2c.h"

typedef struct {
  // High (h) and low (p) voltage (v) and % (p) points.
  float vh, vl, ph, pl;
} batt_disch_linear_section_t;

static const batt_disch_linear_section_t sections[] = {
    {.vh = 3.00f, .vl = 2.90f, .ph = 1.00f, .pl = 0.42f},
    {.vh = 2.90f, .vl = 2.74f, .ph = 0.42f, .pl = 0.18f},
    {.vh = 2.74f, .vl = 2.44f, .ph = 0.18f, .pl = 0.06f},
    {.vh = 2.44f, .vl = 2.01f, .ph = 0.06f, .pl = 0.00f},
};

static float set_battery_percent(float v);
float voltage_to_lux(uint16_t mv);

void read_sensors(sensor_data_t * sen_data) {
  HAL_ADC_Start(&hadc1);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  for (uint8_t i = 0; i < (sizeof(sen_data->RawAdcValues) / sizeof(sen_data->RawAdcValues[0])); i++) {
    HAL_ADC_PollForConversion(&hadc1, 100);
    sen_data->RawAdcValues[i] = HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  sen_data->VRefInt = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(sen_data->RawAdcValues[0], hadc1.Init.Resolution);
  sen_data->Brightness = (uint32_t) voltage_to_lux(__HAL_ADC_CALC_DATA_TO_VOLTAGE(sen_data->VRefInt, sen_data->RawAdcValues[1], hadc1.Init.Resolution) );
  sen_data->BatteryPercent = (uint8_t) (set_battery_percent((float) sen_data->VRefInt / 1000.0f) * 100.0f);
}

void read_i2c_sensor(sensor_data_t * sen_data) {
  int32_t temperature_milli_degC = 0;
  int32_t humidity_milli_RH = 0;

  sht4x_measure_high_precision(&temperature_milli_degC,&humidity_milli_RH);
  sen_data->Temperature = temperature_milli_degC;
  sen_data->Humidity = humidity_milli_RH / 1000;
  sht4x_soft_reset();
}

static float set_battery_percent(float v) {
  if (v > sections[0].vh) {
    return 1.0f;
  }
  for (int i = 0; i < sizeof(sections); i++) {
    const batt_disch_linear_section_t *s = &sections[i];
    if (v > s->vl) {
      return s->pl + (v - s->vl) * ((s->ph - s->pl) / (s->vh - s->vl));
    }
  }
  return 0.0f;
}

float voltage_to_lux(uint16_t mv) {
  if (mv < 70) {
    return 0;
  }

  if (ENABLE_ALTERNATE_LUX_FORMULA != 1) {
    float sensitivity = 0.000396f; // Empfindlichkeit bei 3.3V in A/Lux
    float current = mv / 470.0f; // I = V / R, R = 470 Ohm
    float lux = current / sensitivity;
    return lux * 100;
  }

  mv -= 70;
  float lux_sun = 12000.0f;
  float current_sun = 3.59e-3f;
  float current = ((float) mv / 1000.0f) / 470.0f;
  return MAX(0, MIN(lux_sun * current / current_sun, UINT16_MAX));
}



