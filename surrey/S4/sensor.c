#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"
#include <stdio.h> /* For printf() */

static void print_float(const char *input, float number) {
    int integer_part = (int)number;
    int decimal_part = (int)((number - integer_part) * 1000); // Get the three decimal places
    printf("%s Reading: %d.%03d\n", input, integer_part, decimal_part);
}

float getTemperature(void)
{
  float tempData;

  // NOTE: You only need to use one of the following
  // If you run the code in Cooja Simulator, please remove the second one
  tempData = sht11_sensor.value(SHT11_SENSOR_TEMP_SKYSIM); // For Cooja Sim
//  tempData = sht11_sensor.value(SHT11_SENSOR_TEMP); // For XM   1000 mote
  float d1 = -39.6;
  float d2 = 0.04;
  float temp = tempData * d2 + d1;
  return temp;
}

float getLight(void)
{
  int lightData = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);  
  float V_sensor = 1.5 * lightData /4096;
  float I = V_sensor / 100000; 
  float light = 0.625*1e6*I*1000; 
  return light;
}


/*---------------------------------------------------------------------------*/
PROCESS(sensor_reading_process, "Sensor reading process");
AUTOSTART_PROCESSES(&sensor_reading_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_reading_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_CONF_SECOND/4); // you need to put the correct 
                                           // timer setting here
  SENSORS_ACTIVATE(light_sensor);
  SENSORS_ACTIVATE(sht11_sensor);

  static int counter = 0;
  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev=PROCESS_EVENT_TIMER);

    float temp = getTemperature();
    float light_lx = getLight();
    print_float("Temp", temp);
    print_float("Light", light_lx);
    printf("%d\n", ++counter); // you should also print the temperature
                               // and light intensity here

    etimer_reset(&timer);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

