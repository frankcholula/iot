#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"
#include "dev/button-sensor.h" // For button
#include "dev/leds.h"    // For LED
#include <stdio.h>  // For printf()

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
PROCESS(alarm, "alarm with button");
AUTOSTART_PROCESSES(&alarm);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(alarm, ev, data)
{
  static struct etimer timer;
  static float  tempTh = 28.0; // Default threshold

  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_CONF_SECOND/4);

  SENSORS_ACTIVATE(light_sensor);  // need this for temperature sensor in Simulation
  SENSORS_ACTIVATE(sht11_sensor);
  SENSORS_ACTIVATE(button_sensor); // activate button too
  leds_off(LEDS_ALL);

  while(1) 
  {
    PROCESS_WAIT_EVENT(); // wait for an event

    if (ev==PROCESS_EVENT_TIMER)
    {
      float temp = getTemperature();
      printf("\n%d.%03u C  (threshold = %d.%03u C)", d1(temp), d2(temp),
                                                d1(tempTh), d2(tempTh));

      if(temp>tempTh)
      {
        printf(" -- Alarm!");
        leds_on(LEDS_ALL); // Turn on LEDs
      }

      etimer_reset(&timer);
    }
    else if (ev==sensors_event && data==&button_sensor)
    {
      // When a button is pressed, temp threshold is adjusted
      // and LEDs are turned off
      float temp = getTemperature();
      tempTh = temp + 0.5;

      leds_off(LEDS_ALL); // Turn off LEDs
      printf("\nSetting threshold to %d.%03u C)", d1(tempTh), d2(tempTh));
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/