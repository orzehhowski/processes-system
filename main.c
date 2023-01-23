#include "lib.h"

int main(int argc, char *argv[])
{
  checkInit(argc, argv);

  char queueName[MAX_QUEUE_NAME];
  strcpy(queueName, argv[1]);

  int queueKey = readKeyFromConfig(queueName);
  if (queueKey == -1)
  {
    exit(1);
  }

  int queueId = createProcessQueue(queueKey);
  if (queueId == -1)
  {
    exit(1);
  }

  if (fork())
  {
    struct msgbuf messageBuf;
    int go = 1;
    while (go)
    {
      close(0);
      waitForMessages(queueId, &messageBuf);
      // open given fifo
      char ans[MAX_COMMANDS_SIZE];
      int id = open(messageBuf.fifoName, O_RDONLY);
      read(id, ans, MAX_COMMANDS_SIZE);
      printf("%s\n", ans);
      // execute command
      // save results to fifo
    }
  }
  else
  {
    int go = 1;
    while (go)
    {
      char requestText[MAX_REQUEST] = "";

      // wait for user input
      read(0, requestText, MAX_REQUEST);

      struct request requestBody;
      int splittingRes = splitRequest(requestText, &requestBody);
      if (splittingRes == -1)
      {
        continue;
      }

      printf("1. %s\n2. %s\n3. %s\n", requestBody.queueName, requestBody.commands, requestBody.fifoName);

      // create fifo
      int wasMade = mkfifo(requestBody.fifoName, 0640);
      if (wasMade == -1)
      {
        perror("fifo creating error");
        continue;
      }

      // send request (fifo name) to queue of given process
      int wasSent = sendFifoName(requestBody.fifoName, requestBody.queueName);
      if (wasSent == -1)
      {
        continue;
      }

      // save commands to do in fifo
      int deskToSend = open(requestBody.fifoName, O_WRONLY);
      if (deskToSend == -1)
      {
        perror("opening fifo error");
        continue;
      }
      int wasWrite = write(deskToSend, &requestBody.commands, MAX_COMMANDS_SIZE);
      if (wasWrite == -1)
      {
        close(deskToSend);
        perror("write to fifo error");
        continue;
      }
      close(deskToSend);

      // wait for results given through fifo and print it

      char response[MAX_RESPONSE];

      int wasResponse = receiveResponse(requestBody.fifoName, response);
      if (wasResponse == -1)
      {
        continue;
      }

      write(1, response, MAX_RESPONSE);
    }
  }
}