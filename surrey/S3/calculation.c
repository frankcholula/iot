#include "contiki.h"

#include <stdio.h>  // For printf()
#include <random.h> // for random_rand()

/*---------------------------------------------------------------------------*/
PROCESS(calculation, "Calculation");
AUTOSTART_PROCESSES(&calculation);
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
      printf("The square root of the value = ???\n");

      etimer_reset(&timer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
