#include "contiki.h"
#include "net/rime.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"

#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
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

float getLight(void)
{
  float V_sensor = 1.5 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC)/4096;
                // ^ ADC-12 uses 1.5V_REF
  float I = V_sensor/100000;         // xm1000 uses 100kohm resistor
  float light_lx = 0.625*1e6*I*1000; // convert from current to light intensity
  return light_lx;
}
/*---------------------------------------------------------------------------*/
/* The handshake:
 *
 *             CLIENT           SERVER
 *    (c_state 0) |                 | (s_state 0)
 *                |                 |
 *   timer event, |                 |
 *    do greeting |----["HELLO"]--->| create resources
 * (c_state 0->1) |                 | 
 *                |<---[max_num]----|
 * (c_state 1->2) |                 |
 *                |                 |
 *   timer event, |                 | (s_state 0->1)
 *    read & send |                 |
 *    sensor data |--[sensor_data]->|
 *                |<----["ACK"]-----|
 *                |                 |
 *   timer event, |       ...       |
 *      done with |                 |
 * sensor sending |----["DONE"]---->|
 * (c_state 2->3) |                 |
 *                |<---["READY"]----|
 *       ask for  |                 | (s_state 1->2)
 *       average  |--["AVERAGE"]--->| calculate average
 * (c_state 3->4) |                 |
 *                |<---[average]----|
 * (c_state 4->5) |                 |
 *  close session |-----["END"]---->| free resources
 *                |<----["ACK"]-----|
 * (c_state 5->0) |                 | (s_state 2->0)
 */

struct InformMaxNum {
  char  header[9]; // header signature: "MAX_DATA"
  int   data;
};
struct ReportData {
  char  header[5]; // header signature: "DATA"
  float data;
};
struct ReportOutcome {
  char  header[8]; // header signature: "OUTCOME"
  float data;
};

static int c_state = 0;
static int maxData = 0;
static float average = 0.0;

static rimeaddr_t serverAddr;

static struct unicast_conn uc;

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  // Message from server?
  if(rimeaddr_cmp(from,&serverAddr)==0) return; // return if not

  // retrieve the data as a string first
  char *dataReceived = (char *)packetbuf_dataptr();
  dataReceived[packetbuf_datalen()] = 0;

  if (c_state==1)
  {
    struct InformMaxNum *packet = dataReceived;
    if (strcmp(packet->header,"MAX_DATA")==0)
    {
      maxData = packet->data;
      c_state = 2;
    }
  }
  else if (c_state==2)
  {
    printf("'%s' received\n",dataReceived);
  }
  else if (c_state==3)
  {
    if (strcmp(dataReceived,"READY")==0)
    {
      packetbuf_copyfrom("AVERAGE",7);
      unicast_send(&uc, &serverAddr);
      c_state = 4;
    }
  }
  else if (c_state==4)
  {
    struct ReportOutcome *packet = dataReceived;
    if (strcmp(packet->header,"OUTCOME")==0)
    {
      average = packet->data;
      packetbuf_copyfrom("END",3);
      unicast_send(&uc, &serverAddr);
      printf("Calculated average received = %d.%d\n",d1(average),d2(average));
      c_state = 5;
    }      
  }
  else if (c_state==5)
  {
    if (strcmp(dataReceived,"ACK")==0) c_state = 0;
  }
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  printf("I'm a client mote, my rime addr is: %d.%d\n",
           rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1]);
  unicast_open(&uc, 146, &unicast_callbacks);

  serverAddr.u8[0] = 1;  // this is the server rime address (part 1)
  serverAddr.u8[1] = 0;  // this is the server rime address (part 2)

  SENSORS_ACTIVATE(light_sensor);
  SENSORS_ACTIVATE(sht11_sensor);

  static int numDataSent = 0;

  while(1) 
  {
    static struct etimer et;
    etimer_set(&et, CLOCK_SECOND*5/3);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // timer event, triggering:
    // - GREETING in c_state 0
    // - read & send sensor data in c_state 2
    // - read done also in c_state 2 when enough samples are sent
    if (c_state==0)
    {
      // send greeting
      packetbuf_copyfrom("HELLO",5);
      unicast_send(&uc, &serverAddr);
      printf("HELLO sent.\n");
      numDataSent = 0;
      c_state = 1;
    }
    else if (c_state==2)
    {
      if (numDataSent<5)
      {
        // send sensor data
        struct ReportData packet;
        strcpy(packet.header,"DATA");
        packet.data = getLight();
        packetbuf_copyfrom(&packet,sizeof(packet));
        unicast_send(&uc, &serverAddr);
        printf("Data sent: %u.%03u\n",d1(packet.data),d2(packet.data));
        numDataSent++;
      }
      else
      {
        // send "DONE"
        packetbuf_copyfrom("DONE",4);
        unicast_send(&uc, &serverAddr);
        c_state = 3;
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
