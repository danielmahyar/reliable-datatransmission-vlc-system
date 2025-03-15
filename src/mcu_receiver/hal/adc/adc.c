#include "adc.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_CHANNEL ADC1_CHANNEL_3

// #if CONFIG_IDF_TARGET_ESP32
// static adc_channel_t channel[2] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
// #else
// static adc_channel_t channel[2] = {ADC_CHANNEL_2, ADC_CHANNEL_3};
// #endif

// static TaskHandle_t s_task_handle;
// static const char *TAG = "EXAMPLE";
// static uint32_t data;

// static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
// {
//     BaseType_t mustYield = pdFALSE;
//     // Notify that ADC continuous driver has done enough number of conversions
//     vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

//     return (mustYield == pdTRUE);
// }

// static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
// {
//     adc_continuous_handle_t handle = NULL;

//     adc_continuous_handle_cfg_t adc_config = {
//         .max_store_buf_size = 1024,
//         .conv_frame_size = EXAMPLE_READ_LEN,
//     };
//     ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

//     adc_continuous_config_t dig_cfg = {
//         .sample_freq_hz = 20 * 1000,
//         .conv_mode = EXAMPLE_ADC_CONV_MODE,
//         .format = EXAMPLE_ADC_OUTPUT_TYPE,
//     };

//     adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
//     dig_cfg.pattern_num = channel_num;
//     for (int i = 0; i < channel_num; i++)
//     {
//         adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
//         adc_pattern[i].channel = channel[i] & 0x7;
//         adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
//         adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

//         ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
//         ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
//         ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
//     }
//     dig_cfg.adc_pattern = adc_pattern;
//     ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

//     *out_handle = handle;
// }

// void continuous_read_adc()
// {
//         data = 0;
//     s_task_handle = xTaskGetCurrentTaskHandle();

//     adc_continuous_handle_t handle = NULL;
//     continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

//     adc_continuous_evt_cbs_t cbs = {
//         .on_conv_done = s_conv_done_cb,
//     };
//     ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
//     ESP_ERROR_CHECK(adc_continuous_start(handle));

//     esp_err_t ret;
//     uint32_t ret_num = 0;
//     uint8_t result[EXAMPLE_READ_LEN] = {0};
//     memset(result, 0xcc, EXAMPLE_READ_LEN);
//     while (1)
//     {

//     printf("continuous_read_adc\n");
//         /**
//          * This is to show you the way to use the ADC continuous mode driver event callback.
//          * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
//          * However in this example, the data processing (print) is slow, so you barely block here.
//          *
//          * Without using this event callback (to notify this task), you can still just call
//          * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
//          */
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

//         while (1)
//         {
//             ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
//             if (ret == ESP_OK)
//             {
//                 for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES)
//                 {
//                     adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
//                     uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
//                     uint32_t d = EXAMPLE_ADC_GET_DATA(p);
//                     /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
//                     if (chan_num == 3)
//                     {
//                         data = d;
//                     }
//                     else
//                     {
//                     }
//                 }
//                 /**
//                  * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
//                  * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
//                  * usually you don't need this delay (as this task will block for a while).
//                  */
//                 vTaskDelay(1);
//             }
//             else if (ret == ESP_ERR_TIMEOUT)
//             {
//                 // We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
//                 ESP_LOGW(TAG, "Timeout");
//                 break;
//             }
//         }
//     }
// }

// uint32_t adc_val = 0;

// void read_bhs(){
//     while(1){
//         adc_val = adc1_get_raw(ADC_CHANNEL_3);
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }

static adc_oneshot_unit_handle_t adc1_handle;

void setup_adc()
{
    /*adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_5, &config));*/

    adc1_config_width(ADC_BITWIDTH_12);
    adc1_config_channel_atten(ADC_CHANNEL_5,ADC_ATTEN_DB_0);
}

uint32_t read_adc()
{
    uint32_t adc_reading = adc1_get_raw(ADC_CHANNEL_5);

    return  adc_reading ;
}