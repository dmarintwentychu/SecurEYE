/*
 * i2s_acc.c
 *
 *  Created on: 2020/09/10
 *      Author: nishi
 *
 *  https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/peripherals/i2s.html#_CPPv417i2s_comm_format_t
 */

#include "freertos/FreeRTOS.h"
#include <string.h>

#include "driver/i2s.h"
#include "esp_log.h"
#include "imports/i2s_acc.h"
#include <math.h>

// #include "esp_dsp.h"

#include "esp_sleep.h"


#include "imports/config.h"

#include "imports/jpeg_http_stream.h"

#define DBTHRESHOLD 10
#define TIME_2_SLEEP_IN_SEG 60

const int minutes2check[] = {1, 5};


typedef struct
{
  QueueHandle_t *sound_queue;
  int sound_buffer_size; // in bytes
} src_cfg_t;

static src_cfg_t datapath_config;

QueueHandle_t sndQueue;

static const char *TAG = "I2S_ACC";

int ej = 0;

void pcm_to_db(int16_t *pcm_samples, float *db_values, int num_samples)
{
  float rms_sum = 0.0f;

  // Calcular la suma de los cuadrados de las muestras
  for (int i = 0; i < num_samples; i++)
  {
    rms_sum += pcm_samples[i] * pcm_samples[i];
  }

  // Calcular el valor RMS
  float rms = sqrtf(rms_sum / num_samples);

  // Convertir a decibelios
  *db_values = 20.0f * log10f(rms);
}

void i2s_init(void)
{
  i2s_config_t i2s_config = {
      .mode = I2S_MODE_MASTER | I2S_MODE_RX,        // the mode must be set according to DSP configuration
      .sample_rate = SAMPLE_RATE_HZ,                // must be the same as DSP configuration
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // must be the same as DSP configuration
      .bits_per_sample = 32,                        // must be the same as DSP configuration
      .communication_format = I2S_COMM_FORMAT_I2S,
      .dma_buf_count = 3,
      .dma_buf_len = 300,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = 26,   // IIS_SCLK
      .ws_io_num = 32,    // IIS_LCLK
      .data_out_num = -1, // IIS_DSIN
      .data_in_num = 33   // IIS_DOUT
  };
  i2s_driver_install(1, &i2s_config, 0, NULL);
  i2s_set_pin(1, &pin_config);
  i2s_zero_dma_buffer(1);
}

void soundProcessingTask(void *arg)
{
  i2s_init();

  src_cfg_t *cfg = (src_cfg_t *)arg;
  size_t samp_len = cfg->sound_buffer_size * 2 * sizeof(int) / sizeof(int16_t);

  int *samp = malloc(samp_len);

  size_t read_len = 0;

  // Allocate buffers for audio processing

  int16_t *pcm_before_processing = calloc(1, cfg->sound_buffer_size * sizeof(int16_t));

  int time_elapsed = 0;
  float max_db_Value = 0.0;
  int minutes_index = 1;

  while (1)
  {
    i2s_read(1, samp, samp_len, &read_len, portMAX_DELAY);
    for (int x = 0; x < cfg->sound_buffer_size / 4; x++)
    {
      int s1 = ((samp[x * 4] + samp[x * 4 + 1]) >> 13) & 0x0000FFFF;
      int s2 = ((samp[x * 4 + 2] + samp[x * 4 + 3]) << 3) & 0xFFFF0000;
      samp[x] = s1 | s2;
    }
    memcpy(pcm_before_processing, samp, cfg->sound_buffer_size * sizeof(int16_t));

    float db_values;
    pcm_to_db(pcm_before_processing, &db_values, cfg->sound_buffer_size / 4);

    //printf("%f\n", db_values);

    //send2websocket(db_values);
    //sendmqttnoise(db_values);
    send2wp(db_values);
    //Comprobar cuál es el mayor valor
    if (max_db_Value < db_values)
      max_db_Value = db_values;

    //Siempre despues de la primera hece una vuelta menos al bucle (Arreglar mas adelante...)
    if (time_elapsed > minutes2check[minutes_index]) // Minutes2check toma los valores 100 y 500 (1min o 5min)
    {
      if (max_db_Value < DBTHRESHOLD)
      {
        minutes_index = 0;
        // Se procede ha realizar el light sleep durante 1 min
        printf("----Enabling timer wakeup, %ds\n", TIME_2_SLEEP_IN_SEG); //Que se imprima solo una parte está bien porque printf lo almacena en un búfer
        esp_sleep_enable_timer_wakeup(TIME_2_SLEEP_IN_SEG * 1000000);
        esp_light_sleep_start();
        printf("----Finished\n");
      }
      else
      {
        minutes_index = 1;
      }

      time_elapsed = 0;
      max_db_Value = 0.0;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    time_elapsed++;
  }

  vTaskDelete(NULL);
}

void startI2S(void)
{
  sndQueue = xQueueCreate(2, (AUDIO_CHUNKSIZE * sizeof(int16_t)));
  datapath_config.sound_queue = &sndQueue;
  datapath_config.sound_buffer_size = AUDIO_CHUNKSIZE * sizeof(int16_t);
  xTaskCreatePinnedToCore(&soundProcessingTask, "sound_source", 3 * 1024, (void *)&datapath_config, 5, NULL, 1);
}