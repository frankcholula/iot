#include "contiki.h"

#include <stdio.h>  // For printf()
#include <random.h> // for random_rand()

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

      // We use Babylonian method to calculate x = sqrtroot(S)
      // i.e. iteratively refining x based on x = 0.5*(x+S/x)
      // and we set the maximum number of iterations to 50
      unsigned short S = r;
      float difference = 0.0;
      float error = 0.001;  // error tolerance
      float x = 10.0;       // initial guess
      int   i;
      for (i=0; i<50; i++) // we can affort looping 50 times
      {
          x = 0.5 * (x + S/x);
          difference = x*x - S;
          if (difference<0) difference = -difference;
          if (difference<error) break; // the difference is deemed small enough
      }
      printf("The square root of the value = %d.%03u\n", d1(x), d2(x));
      printf("where %d.%03u^2 = %d.%03u\n", d1(x),d2(x),d1(x*x),d2(x*x));
      printf("The loop takes %u iterations to compute.\n\n", i);

      etimer_reset(&timer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
