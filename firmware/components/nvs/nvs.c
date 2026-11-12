#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

esp_err_t initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        err = nvs_flash_erase();

        if (err != ESP_OK)
        {
            return err;
        }

        err = nvs_flash_init();
    }

    return err;
}