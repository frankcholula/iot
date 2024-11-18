#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"
#include <stdio.h> /* For printf() */

int d1(float f) // Integer part
{
  return((int)f);
}
unsigned int d2(float f) // Fractional part
{
  if (f>0)
    return(1000*(f-d1(f)));
  else
    return(1000*(d1(f)-f));
}

float getTemperature(void)
{
  // For simulation sky mote
  int   tempADC = sht11_sensor.value(SHT11_SENSOR_TEMP_SKYSIM);
  float temp = 0.04*tempADC-39.6; // skymote uses 12-bit ADC, or 0.04 resolution

  // For xm1000 mote
  //int   tempADC = sht11_sensor.value(SHT11_SENSOR_TEMP);
  //float temp = 0.01*tempADC-39.6; // xm1000 uses 14-bit ADC, or 0.01 resolution

  return temp;
}

float getLight(void)
{
  float V_sensor = 1.5 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC)/4096;
                // ^ ADC-12 uses 1.5V_REF
  float I = V_sensor/100000;         // xm1000 uses 100kohm resistor
  float light_lx = 0.625*1e6*I*1000; // convert from current to light intensity
  return light_lx;
}


/*---------------------------------------------------------------------------*/
PROCESS(sensor_reading_process, "Sensor reading process");
AUTOSTART_PROCESSES(&sensor_reading_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_reading_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();

  //etimer_set(&timer, CLOCK_CONF_SECOND/4);
  etimer_set(&timer, CLOCK_CONF_SECOND);   // use this setting for 1 second duration

  SENSORS_ACTIVATE(light_sensor);
  SENSORS_ACTIVATE(sht11_sensor);

  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev=PROCESS_EVENT_TIMER);

    float temp = getTemperature();
    float light_lx = getLight();

    printf("%d.%03u C\n", d1(temp), d2(temp));
    printf("%u.%03u lux\n", d1(light_lx), d2(light_lx));

    etimer_reset(&timer);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
