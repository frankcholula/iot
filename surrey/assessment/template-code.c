#include "contiki.h"
#include <stdio.h> /* For printf() */
#include <random.h>  /* For random_rand() */

#define URN  6891988  // specify your URN here

int d1(float f) // Integer part
{
  return((int)f);
}
unsigned int d2(float f) // Fractional part
{
  if (f>0)
    return(100*(f-d1(f)));
  else
    return(100*(d1(f)-f));
}

float *getTempValues(float *floatArray)
{
  int i;
  for (i=0; i<15; i++)
  {
    unsigned short r = random_rand();
    float randomNum = 50 * ((float)r/RANDOM_RAND_MAX) + 10;
    floatArray[i] = randomNum;
  } 
  return floatArray;
}

void printArray(float *array)
{
  printf("[");
  int i;
  for (i=0; i<14; i++)
    printf("%d.%d, ", d1(array[i]), d2(array[i]));
  printf("%d.%d]\n", d1(array[i]), d2(array[i]));
}

void bubble_sort(float *array, int length) {
  int i, j;
  for (i = 0; i < length - 1; i++) {
    for (j = 0; j < length - i - 1; j++) {
      if (array[j] > array[j + 1]) {
        float temp = array[j];
        array[j] = array[j + 1];
        array[j + 1] = temp;
      }
    }
  }
}

float calculate_median(float *array, int length) {
    bubble_sort(array, length);

    if (length % 2 == 0) {
        return (array[length/2 - 1] + array[length/2]) / 2.0;
    } else {
        return array[length/2];
    }
}


/*---------------------------------------------------------------------------*/
PROCESS(sensor_reading_process, "Sensor reading process");
AUTOSTART_PROCESSES(&sensor_reading_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_reading_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();

  // extract your seed
  unsigned long a = URN;
  unsigned int a3 = a/10000;
  unsigned int a4 = a - a3*10000;
  printf("Your URN is: %d%d\n", a3, a4);
  random_init(a4);

  // generate 15 random temperature values
  float temp15[15];
  getTempValues(temp15);

  // print array B and the average
  int i;
  float average = 0;
  for (i=0; i<15; i++)
    average = average + temp15[i];
  average = average / 15;
  printf("B = ");
  printArray(temp15);
  printf("Average = %d.%d\n", d1(average), d2(average));

  // determine the median (implemente your code here)
  float median;
  median = calculate_median(temp15, 15);

  printf("Median = %d.%d\n", d1(median), d2(median));

  // determine array E (implemente your code here)
  float beta = 0.7;
  float EMA[15];
  EMA[0] = temp15[0];

  for (i=1; i<15; i++)
  {
    EMA[i] = beta * temp15[i] + (1 - beta) * EMA[i-1];
  }

  printf("E = ");
  printArray(EMA);

  // determine Pearson Correlation Coefficient (implemente your code here)
  float pearsonCC = 0;

  printf("Pearson Correlation Coefficient = %d.%d\n", d1(pearsonCC), d2(pearsonCC));


  // do not touch the following
  etimer_set(&timer, CLOCK_CONF_SECOND);   // use this setting for 1 second duration

  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev=PROCESS_EVENT_TIMER);
    etimer_reset(&timer);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
