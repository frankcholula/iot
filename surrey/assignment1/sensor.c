#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"
#include "lib/list.h"
#include "lib/memb.h"
#include <stdio.h> /* For printf() */

#define BUFFER_SIZE 12

/* Helper functions and sensor functions */
static void print_float(float number) {
    int integer_part = (int)number;
    int decimal_part = (int)((number - integer_part) * 1000);
    printf("%d.%03d ", integer_part, decimal_part);
}

float getTemperature(void) {
    float tempData;
    tempData = sht11_sensor.value(SHT11_SENSOR_TEMP_SKYSIM); // For Cooja Sim
    float d1 = -39.6;
    float d2 = 0.04; // For Cooja Sim
    float temp = tempData * d2 + d1;
    return temp;
}

float getLight(void) {
    int lightData = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
    float V_sensor = 1.5 * lightData / 4096;
    float I = V_sensor / 100000;
    float light = 0.625 * 1e6 * I * 1000;
    return light;
}

/* Part 1: Running sum variables */
static float running_sum_temp = 0.0; // Running sum for temperature
static float running_sum_light = 0.0; // Running sum for light    

/* Structure for linked list of sensor data */
struct sensor_data {
    struct sensor_data *next;
    float temperature;
    float light;
};

LIST(sensor_list);
MEMB(sensor_mem, struct sensor_data, BUFFER_SIZE);

/* Add sensor data and maintain running sums */
static void add_sensor_data(float temperature, float light) {
    struct sensor_data *new_data;

    if (list_length(sensor_list) >= BUFFER_SIZE) {
        struct sensor_data *oldest = list_pop(sensor_list);
        running_sum_temp -= oldest->temperature;
        running_sum_light -= oldest->light;
        memb_free(&sensor_mem, oldest);
    }

    new_data = memb_alloc(&sensor_mem);
    if (new_data == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }

    // Fill the data
    new_data->temperature = temperature;
    new_data->light = light;

    // Add the new data to the end of the list
    list_add(sensor_list, new_data);

    // Update the running sum
    running_sum_temp += temperature;
    running_sum_light += light;
}

/* Print all sensor data in the list */
static void print_sensor_list(const char *type) {
    struct sensor_data *item;

    if (strcmp(type, "Temp") == 0) {
        printf("Temp Readings: ");
        for (item = list_head(sensor_list); item != NULL; item = list_item_next(item)) {
            print_float(item->temperature);
        }
    } else if (strcmp(type, "Light") == 0) {
        printf("Light Readings: ");
        for (item = list_head(sensor_list); item != NULL; item = list_item_next(item)) {
            print_float(item->light);
        }
    } else if (strcmp(type, "Both") == 0) {
        printf("Temp and Light Readings: ");
        for (item = list_head(sensor_list); item != NULL; item = list_item_next(item)) {
            printf("[");
            print_float(item->temperature);
            print_float(item->light);
            printf("] ");
        }
    } else {
        printf("Unknown type: %s\n", type);
        return;
    }

    printf("\n");
}

/* Get the average directly from running sums */
static float calculate_average(const char *type) {
    int count = list_length(sensor_list); // Get the number of items in the list
    if (count == 0) {
        return 0.0; // Avoid division by zero
    }

    if (strcmp(type, "Temp") == 0) {
        return running_sum_temp / count;
    } else if (strcmp(type, "Light") == 0) {
        return running_sum_light / count;
    }

    return 0.0; // Unknown type
}

/*---------------------------------------------------------------------------*/
PROCESS(sensor_reading_process, "Sensor reading process");
AUTOSTART_PROCESSES(&sensor_reading_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_reading_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();
    etimer_set(&timer, CLOCK_CONF_SECOND / 2); // Two readings per second
                                           
    SENSORS_ACTIVATE(light_sensor);
    SENSORS_ACTIVATE(sht11_sensor);

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

        float temp = getTemperature();
        float light_lx = getLight();
        
        add_sensor_data(temp, light_lx);

        // Debugging: Print list
        print_sensor_list("Light");

        printf("Average Light: ");
        print_float(calculate_average("Light"));
        printf("\n");

        etimer_reset(&timer);
    }

    PROCESS_END();
}
