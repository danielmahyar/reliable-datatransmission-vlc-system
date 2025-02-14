#include "adc.h"
#include "driver/adc.h"

#define ADC_CHANNEL ADC1_CHANNEL_3

void setup_adc() {
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_12); // ADC1_CHANNEL_0 is GPIO2
}

uint16_t read_adc() {
    // Read ADC
    uint16_t adc_reading = adc1_get_raw(ADC_CHANNEL);
    log(INFORMATION, "ADC Reading: %d", adc_reading);
    return adc_reading;
}