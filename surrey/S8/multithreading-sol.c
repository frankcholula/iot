#include "contiki.h"
#include <stdio.h> /* For printf() */

#include "dev/sht11-sensor.h"
#include "dev/light-sensor.h"

#define MAX_VALUES  4 // Number of values used in averaging process

static process_event_t event_data_ready; // Application specific event value

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
/* We declare the two processes */
PROCESS(temp_process, "Temperature process");
PROCESS(print_process, "Print process");

/* We require the processes to be started automatically */
AUTOSTART_PROCESSES(&temp_process, &print_process);
/*---------------------------------------------------------------------------*/
/* Implementation of the first process */
PROCESS_THREAD(temp_process, ev, data)
{
    /* Variables are declared static to ensure their values are kept */
    /* between kernel calls. */
    static struct etimer timer;
    static int count = 0;
    static float average = 0, valid_measure = 0;

    // This variable is recomputed at every run, therefore it is not
    // necessary to declare them static.
    float measure;

    // Any process must start with this.
    PROCESS_BEGIN();

    // Allocate the required event
    event_data_ready = process_alloc_event();

    // Initialise the temperature sensor
    SENSORS_ACTIVATE(light_sensor);  // need this for sky-mote emulation
    SENSORS_ACTIVATE(sht11_sensor);

    // Initialise variables
    count = 0;
    average = 0;
    valid_measure = 0;
    measure = 0;

    // Set the etimer module to generate an event in 0.25 second
    etimer_set(&timer, CLOCK_CONF_SECOND/4);

    while (1)
    {
        // Wait here for the timer to expire
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

        // Get temperature measurement and convert it to degrees
        measure = getTemperature();
        printf("[temperature process] ");
        printf("temp = %d.%03u\n", d1(measure), d2(measure));

        // Sum temperature measurements & count the measures
       	average += measure;
       	count++;

        // Check if enough samples are collected
        // If so, calculate the average and pass the value to print_process
       	if (count==MAX_VALUES)
       	{
       	  // Average the measure
       	  valid_measure = average / MAX_VALUES;

       	  // Reset variables
       	  average = 0;
       	  count = 0;

       	  // Post an event to the print process
       	  // and pass a pointer to the last measure as data
          printf("[temperature process] ");
          printf("passing value %d.%03u\n", d1(valid_measure), d2(valid_measure));

          process_post(&print_process, event_data_ready, &valid_measure);
        }

        // Reset the timer so it will generate another event
        etimer_reset(&timer);
     }

     // Any process must end with this, even if it is never reached.
     PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* Implementation of the second process */
PROCESS_THREAD(print_process, ev, data)
{
    PROCESS_BEGIN();

    while (1)
    {
      // Wait until we get a data_ready event
      PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

      // Use 'data' variable to retrieve data and then display it
      printf("[print process] data ready\n");
      printf("temperature = %d.%03u\n", d1(*(float *)data), d2(*(float *)data));
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/