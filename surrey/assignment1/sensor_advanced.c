#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"
#include "lib/list.h"
#include "lib/memb.h"
#include <stdio.h> /* For printf() */

#define BUFFER_SIZE 12
#define LOW_ACTIVITY_THRESHOLD 1000
#define HIGH_ACTIVITY_THRESHOLD 5000
#define SAX_FRAGMENTS 4
#define SAX_ALPHABET_SIZE 4

/* Helper functions */
static void print_float(float number)
{
    int integer_part = (int)number;
    int decimal_part = (int)((number - integer_part) * 1000);
    printf("%d.%02d", integer_part, decimal_part);
}

float read_light_sensor(void)
{
    int lightData = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
    float V_sensor = 1.5 * lightData / 4096;
    float I = V_sensor / 100000;
    float light = 0.625 * 1e6 * I * 1000;
    return light;
}

float get_temp_sensor(void)
{
    float tempData = sht11_sensor.value(SHT11_SENSOR_TEMP_SKYSIM); // For Cooja Sim
    float d1 = -39.6;
    float d2 = 0.04; // Adjust as per sensor calibration
    float temp = tempData * d2 + d1;
    return temp;
}

/* Use linked list for sensor data */
struct sensor_data
{
    struct sensor_data *next;
    float value;
};

LIST(light_list);
LIST(temp_list);
MEMB(light_mem, struct sensor_data, BUFFER_SIZE);
MEMB(temp_mem, struct sensor_data, BUFFER_SIZE);

static void add_sensor_data(float value, list_t lst, struct memb *mem)
{
    struct sensor_data *new_data;

    if (list_length(lst) >= BUFFER_SIZE)
    {
        struct sensor_data *oldest = list_pop(lst);
        memb_free(mem, oldest);
    }

    new_data = memb_alloc(mem);
    if (new_data == NULL)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    new_data->value = value;
    list_add(lst, new_data);
}

static float calculate_avg(list_t lst)
{
    struct sensor_data *item;
    float sum = 0.0;
    int count = 0;
    for (item = list_head(lst); item != NULL; item = list_item_next(item))
    {
        sum += item->value;
        count++;
    }
    return (count == 0) ? 0.0 : sum / count;
}

static float calculate_ssd(float avg, list_t lst)
{
    struct sensor_data *item;
    float ssd = 0.0;
    for (item = list_head(lst); item != NULL; item = list_item_next(item))
    {
        float diff = item->value - avg;
        ssd += diff * diff;
    }
    return ssd;
}

/* Calculate standard deviation */
static float sqrt_approx(float ssd)
{
    float error = 0.001; // Error tolerance for Babylonian method
    float x = ssd;       // Initial guess for square root
    float difference;
    int i;

    if (ssd == 0)
    {
        return 0.0; // No variance
    }

    for (i = 0; i < 50; i++)
    {
        x = 0.5 * (x + ssd / x);
        difference = x * x - ssd;
        if (difference < 0)
        {
            difference = -difference;
        }
        if (difference < error)
        {
            break;
        }
    }
    return x;
}

static float calculate_std(list_t lst)
{
    float avg = calculate_avg(lst);
    float ssd = calculate_ssd(avg, lst);
    return sqrt_approx(ssd);
}

/* Advanced features */
static float fabs(float value)
{
    return (value < 0) ? -value : value;
}

static float calculate_manhattan_dist(list_t vec_X, list_t vec_Y)
{
    struct sensor_data *x_item = list_head(vec_X);
    struct sensor_data *y_item = list_head(vec_Y);
    float dist = 0.0;

    while (x_item != NULL && y_item != NULL)
    {
        dist += fabs(x_item->value - y_item->value);
        x_item = list_item_next(x_item);
        y_item = list_item_next(y_item);
    }
    return dist;
}

static float calculate_correlation(list_t vec_X, list_t vec_Y)
{
    struct sensor_data *x_item = list_head(vec_X);
    struct sensor_data *y_item = list_head(vec_Y);

    float avg_x = calculate_avg(vec_X);
    float avg_y = calculate_avg(vec_Y);
    float std_x = calculate_std(vec_X);
    float std_y = calculate_std(vec_Y);

    if (std_x == 0 || std_y == 0)
    {
        printf("Correlation undefined due to zero standard deviation.\n");
        return 0.0;
    }

    float numerator = 0.0;
    while (x_item != NULL && y_item != NULL)
    {
        float x = x_item->value;
        float y = y_item->value;

        numerator += (x - avg_x) * (y - avg_y);

        x_item = list_item_next(x_item);
        y_item = list_item_next(y_item);
    }
    return numerator / (std_x * std_y);
}

// TODO: Implement Short Time Fourier Transform
static void perform_stft(list_t lst) 
{

}




// TODO: Implement Spectral 


static void aggregate_and_report()
{
    struct sensor_data *light_item = list_head(light_list);
    struct sensor_data *temp_item = list_head(temp_list);
    printf("X (Light Sensor Readings) = [");
    for (light_item = list_head(light_list); light_item != NULL; light_item = list_item_next(light_item))
    {
        print_float(light_item->value);
        if (list_item_next(light_item) != NULL)
        {
            printf(", ");
        }
    }
    printf("]\n");
    printf("Y (Temperature Sensor Readings) = [");
    for (temp_item = list_head(temp_list); temp_item != NULL; temp_item = list_item_next(temp_item))
    {
        print_float(temp_item->value);
        if (list_item_next(temp_item) != NULL)
        {
            printf(", ");
        }
    }
    printf("]\n");

    printf("Manhattan Distance: ");
    print_float(calculate_manhattan_dist(light_list, temp_list));
    printf("\n");

    printf("Correlation: ");
    print_float(calculate_correlation(light_list, temp_list));
    printf("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS(sensor_reading_process, "Sensor reading process");
AUTOSTART_PROCESSES(&sensor_reading_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_reading_process, ev, data)
{
    static struct etimer timer;
    static int sample_counter = 0;
    static int k = 12; // number of samples before calculation

    PROCESS_BEGIN();
    etimer_set(&timer, CLOCK_CONF_SECOND / 2); // Two readings per second

    SENSORS_ACTIVATE(light_sensor);
    SENSORS_ACTIVATE(sht11_sensor);

    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

        float light = read_light_sensor();
        float temp = get_temp_sensor();
        add_sensor_data(light, light_list, &light_mem);
        add_sensor_data(temp, temp_list, &temp_mem);

        sample_counter++;
        if (sample_counter >= k)
        {
            aggregate_and_report();
            sample_counter = 0;
        }

        etimer_reset(&timer);
    }

    PROCESS_END();
}
