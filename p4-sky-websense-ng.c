#include "contiki.h"
#include "os/net/app-layer/coap/coap-engine.h"
#include "arch/dev/sensor/sht11/sht11-sensor.h"
#include "arch/platform/sky/dev/light-sensor.h"
#include "os/dev/leds.h"
#include <stdio.h>

static void res_get_temp_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_get_light_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_temp,
         "title=\"Temperature\";rt=\"Temperature\"",
         res_get_temp_handler,
         NULL,
         NULL,
         NULL);

RESOURCE(res_light,
         "title=\"Light\";rt=\"Light\"",
         res_get_light_handler,
         NULL,
         NULL,
         NULL);

PROCESS(sky_websense_process, "Sky Web Sense Process");
AUTOSTART_PROCESSES(&sky_websense_process);

static int get_light(void) {
  return 10 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC) / 7;
}

static int get_temp(void) {
  return ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
}

static void res_get_temp_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  int temp = get_temp();
  int length = snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{\"temperature\": %d}", temp);
  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_payload(response, buffer, length);
}

static void res_get_light_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  int light = get_light();
  int length = snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{\"light\": %d}", light);
  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_payload(response, buffer, length);
}

PROCESS_THREAD(sky_websense_process, ev, data) {
  static struct etimer timer;

  PROCESS_BEGIN();

  // Initialize sensors
  sht11_sensor.configure(SENSORS_HW_INIT, 1);
  light_sensor.configure(SENSORS_HW_INIT, 1);

  // Activate the CoAP resources
  coap_activate_resource(&res_temp, "sensors/temperature");
  coap_activate_resource(&res_light, "sensors/light");

  etimer_set(&timer, CLOCK_SECOND * 2);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);

    // Sensor readings are handled when CoAP resources are accessed
  }

  PROCESS_END();
}
