/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart.h"

#include "user_config.h"
#include "jerry_run.h"
#include "jerry-targetjs.h"

static void show_free_mem(int idx) {
  size_t res = xPortGetFreeHeapSize();
  printf("dbg free memory(%d): %d\r\n", idx, res);
}

static int jerry_task_init(void) {
  DECLARE_JS_CODES;

  js_entry();

  /* run js files */
  show_free_mem(2);
  for (int src = 0; js_codes[src].source; src++) {
    int retcode = js_eval(js_codes[src].source, js_codes[src].length);
    if (retcode != 0) {
      printf("js_eval failed code(%d) [%s]\r\n", retcode, js_codes[src].name);
      return -2;
    }
  }
  show_free_mem(3);
  return 0;
}

static void jerry_task(void *pvParameters) {
  if (jerry_task_init() == 0) {
    const portTickType xDelay = 100 / portTICK_RATE_MS;
    uint32_t ticknow = 0;

    for (;;) {
      vTaskDelay(xDelay);
      js_loop(ticknow);
      if (!ticknow) {
        show_free_mem(4);
      }
      ticknow++;
    }
  }
  js_exit();
}

/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR user_init(void)
{
  UART_SetBaudrate(UART0, BIT_RATE_115200);

  show_free_mem(0);
  wifi_softap_dhcps_stop();
  show_free_mem(1);

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);    // GPIO 0
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);    // GPIO 2

  xTaskCreate(jerry_task, "jerry", JERRY_STACK_SIZE, NULL, 2, NULL);
}

/*
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reserved 4 sectors, used for rf init data and Parameters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 */
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
