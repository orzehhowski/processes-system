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

  if (fork())
  {
    struct msgbuf messageBuf;
    int go = 1;
    while (go)
    {
      int queueId = createProcessQueue(queueKey);
      if (queueId == -1)
      {
        exit(1);
      }

      printf("[1] waiting for message from other process\n");

      waitForMessages(queueId, &messageBuf);

      printf("[1] message received, reading from fifo %s\n", messageBuf.fifoName);

      // open given fifo

      char commands[MAX_COMMANDS_SIZE];
      int wasReaden = readFromFifo(messageBuf.fifoName, commands, MAX_COMMANDS_SIZE);
      if (wasReaden == -1)
      {
        continue;
      }
      printf("[1] request received: %s\n", commands);

      // execute command
      int execRes = executeCommands(commands, messageBuf.fifoName);
      if (execRes == -1)
      {
        continue;
      }
    }
  }
  else
  {
    int go = 1;
    while (go)
    {
      char requestText[MAX_REQUEST] = "";

      // wait for user input
      printf("[2] waiting for user input\n");
      read(0, requestText, MAX_REQUEST);

      struct request requestBody;
      int splittingRes = splitRequest(requestText, &requestBody);
      if (splittingRes == -1)
      {
        continue;
      }

      printf("[2] input received:\n    call process %s to execute %s and send results through fifo %s\n",
             requestBody.queueName, requestBody.commands, requestBody.fifoName);

      // create fifo
      printf("[2] creating fifo %s\n", requestBody.fifoName);
      int wasMade = mkfifo(requestBody.fifoName, 0640);
      if (wasMade == -1)
      {
        perror("fifo creating error");
        continue;
      }

      // send request (fifo name) to queue of given process

      printf("[2] sending fifo name to %s\n", requestBody.queueName);
      int wasSent = sendFifoName(requestBody.fifoName, requestBody.queueName);
      if (wasSent == -1)
      {
        continue;
      }

      // save commands to do in fifo
      printf("[2] saving commands [%s] to fifo %s\n", requestBody.commands, requestBody.fifoName);
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
      printf("[2] waiting for response\n");
      int wasResponse = readFromFifo(requestBody.fifoName, response, MAX_RESPONSE);
      if (wasResponse == -1)
      {
        continue;
      }

      unlink(requestBody.fifoName);

      printf("[2] response received\n");
      write(1, response, strlen(response));
      write(1, "\n\n", 3);
    }
  }
}
