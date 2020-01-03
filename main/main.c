#include <stdio.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "esp_log.h"

#include "m5stickc.h"
#include "sdkconfig.h"
#include "wire.h"
#include "AXP192.h"
#include "driver/i2s.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"

#define SNAP_LEN	2324

uint32_t deauths = 0;
uint32_t tmpPacketCounter = 0;
int32_t rssiSum;
uint8_t wfchn = 1;

void change_chn(uint8_t);

void wifi_promiscuous(void *buf, wifi_promiscuous_pkt_type_t type)
{
	wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t* )buf;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t) pkt->rx_ctrl;

	if (type == WIFI_PKT_MGMT &&
			(pkt->payload[0] == 0xA0 ||
			 pkt->payload[0] == 0xC0
			)
	   )
	{
		deauths ++;
	}

	if (type == WIFI_PKT_MISC ||			// Wrong packet type
			ctrl.sig_len > SNAP_LEN		// Packet too long
	   )
	{
		return;
	}

	tmpPacketCounter++;
	rssiSum += ctrl.rssi;
	rssiSum /= 2;

	// TFT_print("0", CENTER, 1);
	//printf("count: %d, rssi: %d", tmpPacketCounter, rssiSum);
	// ESP_LOGI("main", "len: %d", ctrl.sig_len);
	// ESP_LOGI("main", "rssi: %d", ctrl.rssi);
	return;
}

void btnEvt(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
	if ((base == button_a.esp_event_base) && (id == BUTTON_PRESSED_EVENT))
	{
		ESP_LOGI("main", "A pressed");
		if (wfchn >= 14) wfchn = 1;
		else wfchn += 1;

		TFT_print("                ", 5, 20);
		change_chn(wfchn);
	}

	if ((base == button_b.esp_event_base) && (id == BUTTON_PRESSED_EVENT))
	{
		ESP_LOGI("main", "B pressed");
	}
}

static void WiFi_Sniffer(void)
{
	char strbuff[255] = {0};
	
	for (;;)
	{
		sprintf(strbuff, "rssi = %d", rssiSum);
		TFT_print(strbuff, 5, 40);
		sprintf(strbuff, "chn = %d", wfchn);
		TFT_print(strbuff, 5, 20);
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}

static void stickc_task(void *arg)
{
	for (;;)
	{
		WiFi_Sniffer();
	}
}

void change_chn(uint8_t nChn)
{
	esp_wifi_set_promiscuous(false);
	esp_wifi_set_channel(nChn, WIFI_SECOND_CHAN_NONE);
	esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
	esp_wifi_set_promiscuous(true);
}

void app_main(void)
{
	M5Init();

	esp_event_handler_register_with(event_loop, BUTTON_A_EVENT_BASE, BUTTON_PRESSED_EVENT, btnEvt, NULL);
	esp_event_handler_register_with(event_loop, BUTTON_B_EVENT_BASE, BUTTON_PRESSED_EVENT, btnEvt, NULL);

	TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE);
	TFT_setFont(SMALL_FONT, NULL);
	TFT_resetclipwin();

	nvs_flash_init();
	tcpip_adapter_init();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();

	change_chn(wfchn);

	TFT_print(">>>Hello World<<<", CENTER, 0);
	
	xTaskCreate(stickc_task, "stickc_task", 1024 * 2, (void*) NULL, 10, NULL);
	return;
}
