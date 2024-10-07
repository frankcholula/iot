#include "contiki.h"

#include <stdio.h>  // For printf()
#include <random.h> // for random_rand()
#include <math.h>   // for fabs()

/*---------------------------------------------------------------------------*/
PROCESS(calculation, "Calculation");
AUTOSTART_PROCESSES(&calculation);

static void print_float(float number){
    int integer_part = (int)number;
    int decimal_part = (int)((number - integer_part) * 1000); // Get the two decimal places
    printf("The square root of the value =: %d.%02d\n", integer_part, decimal_part);
}

static float babylonian_sqrt(float r) {
  float x = r;
  float y = 0;
  float z = r / 2.0f;  // Initial guess
  float epsilon = 0.00001f;  // Convergence threshold
  while (fabs(y - z) >= epsilon) {
    y = z;
    z = (x / y + y) / 2;
  }
  return y;  // Return the square root as a float
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(calculation, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_CONF_SECOND*3);

  while(1) 
  {
    PROCESS_WAIT_EVENT(); // wait for an event

    if (ev==PROCESS_EVENT_TIMER)
    {
      unsigned short r = random_rand()/4;
      printf("The random value is %d\n", r);
      float sqrt = babylonian_sqrt(r);
      print_float(sqrt);
      etimer_reset(&timer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
