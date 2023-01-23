#include "lib.h"

int main(int argc, char *argv[])
{
  checkInit(argc, argv);

  char queueName[MAX_QUEUE_NAME];
  strcpy(queueName, argv[1]);

  int queueKey = readKeyFromConfig(queueName);

  int queueId = createProcessQueue(queueKey);

  if (fork())
  {
    struct msgbuf messageBuf;
    int go = 1;
    while (go)
    {
      close(0);
      waitForMessages(queueId, &messageBuf);
      // open given fifo
      // execute command
      // save results to fifo
    }
  }
  else
  {
    int go = 1;
    while (go)
    {
      char requestBody[MAX_REQUEST] = "";

      // wait for user input
      read(0, requestBody, MAX_REQUEST);

      struct request requestBuf;
      int splittingRes = splitRequest(requestBody, &requestBuf);
      if (splittingRes == -1)
      {
        continue;
      }
      printf("1. %s\n2. %s\n3. %s\n", requestBuf.queueName, requestBuf.commands, requestBuf.fifoName);

      // create fifo and save there commands to do
      // send request (fifo name) to queue of given process
      // wait for results given through fifo and print it
    }
  }
}