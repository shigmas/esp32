#include <stdio.h>

#include "BLEBroadcaster.h"
#include "BLEDummyEmitter.h"

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
    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();
    /* Clean up at exit */
    vTaskDelete(NULL);
}

static void broadcast_task(void *param) {
    BLEBroadcaster *b = (BLEBroadcaster *)param;
    /* Task entry log */
    ESP_LOGI(TAG, "broadcast_task!");

    while (1) {
        /* Sleep */
        vTaskDelay(b->GetBroadcastInterval());
    }

    /* Clean up at exit */
    vTaskDelete(NULL);
}
void app_main(void)
{
    BLEBroadcaster *b = new BLEBroadcaster("blab_tester");
    BLEEmitter *e = new BLEDummyEmitter("DummyEmitter");
    b->AddEmitter(e);
    b->Init();

    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    xTaskCreate(broadcast_task, "NimBLE Broadcast", 4*1024, b, 5, NULL);
    return;
}
