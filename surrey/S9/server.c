#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>
#include <string.h>

#include "cipher.c" // include the source

static struct unicast_conn uc;
static int dialogState = 0; // 0:listening, 1:greeting_done
static rimeaddr_t clientAddr;
static int numExchange = 0;
static int shiftKey = 7;

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example secured unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  char *dataReceived = (char *)packetbuf_dataptr();
  dataReceived[packetbuf_datalen()] = 0;

  if (dialogState==0)
  {
    // Valid greeting?
    if (strcmp(dataReceived,"HELLO")==0)
    {
      // then reply to the client
      rimeaddr_copy(&clientAddr,from);
      packetbuf_copyfrom("message?",8);
      unicast_send(&uc, &clientAddr);
      printf("Greeting from %d.%d received and responded.\n",from->u8[0],from->u8[1]);
      dialogState = 1; // greeting done, move on to the next state
    }
  }
  else if (dialogState==1)
  {
    // From the same client?
    if (rimeaddr_cmp(&clientAddr,from)!=0)
    {
      // then process the received message
      char strbuf[100];
      decrypt(dataReceived, strbuf, shiftKey);
      printf("Message '%s' from %d.%d arrived\n",dataReceived,from->u8[0],from->u8[1]);
      printf("The decrypted message is: '%s'\n",strbuf);

      // and reply OK to end the dialog
      packetbuf_copyfrom("OK",2);
      unicast_send(&uc, &clientAddr);
      printf("'OK' replied\n");
      dialogState = 0; // message received, reset back to listening state
      numExchange++;
    }
  }
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  printf("I'm the server, my rime addr is: %d.%d\n",
           rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1]);
  unicast_open(&uc, 146, &unicast_callbacks);

  while(1)
  {
    static struct etimer et;
    etimer_set(&et, CLOCK_SECOND*5);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // Print some statistic
    printf("Number of messages received = %d\n",numExchange);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
