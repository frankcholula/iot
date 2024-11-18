#include "contiki.h"

#include <stdio.h>   /* For printf() */
#include <random.h>  /* For random_rand() */


/*
 * Contiki library does not support floating point printing
 * We can use the following functions to convert a floating
 * point to two integers, one represents the integer part
 * and another represents the fractional part.
 * 
 * Note: The implementation fails to give correct output
 * when the input f is 0<f<-1. The function d1() returns 
 * 0 which is shown as a positive value rather than a 
 * negative one. You can try with f=-0.5;
 * Can you improve the implementation?
 */
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
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Hello, world\n"); // This is "Hello World" program

  // Now, we implement logging random numbers to console
  int i;
  float randomNum, randomSum = 0.0;
  printf("Random Numbers:\n");
  for (i=0; i<10; i++)
  {
    unsigned short r = random_rand();
    randomNum = 2.5 * ((float)r/RANDOM_RAND_MAX);
    randomSum += randomNum;
    printf("%d.%03u\n",d1(randomNum),d2(randomNum));
  }

  printf("Sum = %d.%03u\n",d1(randomSum),d2(randomSum));

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/