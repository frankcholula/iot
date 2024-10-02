#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>
#include <string.h>

#include "cipher.c" // include the source

static int dialogState = 0; // 0:waiting_greeting_ack, 1:waiting_dialog_end
static rimeaddr_t serverAddr;
static char *myMessage = "job done";
static int shiftKey = 7;
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

  char *dataReceived = (char *)packetbuf_dataptr();
  dataReceived[packetbuf_datalen()] = 0;

  if (dialogState==0) // waiting for greeting ACK?
  {
    // Valid greeting reply?
    if (strcmp(dataReceived,"message?")==0)
    {
      // then send an encrypted message to the server
      char strbuf[100];
      encrypt(myMessage, strbuf, shiftKey);
      packetbuf_copyfrom(strbuf,strlen(myMessage));
      unicast_send(&uc, &serverAddr);
      printf("Message '%s' sent with encryption\n",myMessage);
      dialogState = 1; // encrypted message sent, move on to the next state
    }
  }
  else if (dialogState==1) // waiting for dialog end confirmation?
  {
    // Valid dialog end confirmation?
    if (strcmp(dataReceived,"OK")==0)
    {
      dialogState = 0; // dialog end confirmation received, reset to state 0
    }
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

  while(1) 
  {
    static struct etimer et;

    etimer_set(&et, CLOCK_SECOND*5/7);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if (dialogState==0)
    {
      packetbuf_copyfrom("HELLO",5);
      unicast_send(&uc, &serverAddr);
      printf("'HELLO' sent to %d.%d.\n",serverAddr.u8[0],serverAddr.u8[1]);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
