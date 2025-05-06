#include <stdio.h>

#include "BLEBroadcaster.h"

#include "nimble/nimble_port.h"

#include "esp_log.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" {
    void app_main(void);
}

#define TAG "blab_main"

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}

void app_main(void)
{
    BLEBroadcaster b("blab_tester");
    b.Init();


    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    return;
}
