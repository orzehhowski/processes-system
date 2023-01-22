#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#define MAX_QUEUE_NAME 64
#define MAX_KEY_STR 16
#define MAX_REQUEST 192
#define MAX_COMMANDS_SIZE 64
#define MAX_FIFO_NAME 64

struct msgbuf
{
  long mtype;
  char request[MAX_REQUEST];
};

struct request
{
  char queueName[MAX_QUEUE_NAME];
  char commands[MAX_COMMANDS_SIZE];
  char fifoName[MAX_FIFO_NAME];
};

// size as global var cuz why not
int MSG_SIZE = sizeof(struct msgbuf) - sizeof(long);

// Checks if program is runned correctly
void checkInit(int argc, char *argv[])
{
  if (argc != 2)
  {
    write(2, "wrong number of args, must be 2\n", 33);
    exit(1);
  }

  if (strlen(argv[1]) > MAX_QUEUE_NAME)
  {
    write(2, "Queue name to long\n", 20);
    exit(1);
  }
}

int readKeyFromConfig(char queueName[])
{
  int configFd = open("./config.txt", O_RDONLY);
  if (configFd == -1)
  {
    perror("opening config file error");
    exit(1);
  }

  int readValue = 1;
  while (readValue > 0)
  {
    int i = -1;
    char nameBuf[MAX_QUEUE_NAME];

    // read name from config file
    while ((nameBuf[i++] != ':') && (readValue > 0))
    {
      readValue = read(configFd, &nameBuf[i], 1);
    }
    if (readValue == -1)
    {
      perror("Read config file error");
      exit(1);
    }

    // Delete ':' from name
    if (i > 0)
    {
      nameBuf[i - 1] = '\0';
    }

    // if correct queue name was found
    if (!strcmp(queueName, nameBuf))
    {
      // read queue t_key
      char keyBuf[MAX_KEY_STR];
      i = -1;
      while ((keyBuf[i++] != '\n') && (readValue > 0))
      {
        readValue = read(configFd, &keyBuf[i], 1);
      }
      if (readValue == -1)
      {
        perror("Read config file error");
        exit(1);
      }
      close(configFd);
      return atoi(keyBuf);
    }

    // else go to next line
    char buf;
    while (buf != '\n')
    {
      read(configFd, &buf, 1);
    }
  }
  // if couldn't find matching name
  close(configFd);
  write(2, "couldn't find matching queue name in config file\n", 50);
  exit(1);
}

// returns ID of created queue
int createProcessQueue(int key)

{
  int QueueId = msgget(key, IPC_CREAT | 0640);
  if (QueueId == -1)
  {
    perror("Create process queue error");
    exit(1);
  }
  return QueueId;
}

int waitForMessages(int queueId, struct msgbuf *messageBuf)
{
  int received = msgrcv(queueId, messageBuf, MSG_SIZE, 1, 0);
  if (received == -1)
  {
    perror("receiving message error");
    exit(1);
  }
  return received;
}

int splitRequest(char request[], struct request *requestBuf)
{
  int requestSize = strlen(request);
  int current = 0;
  char buf;

  // read first arg
  char queueNameBuf[MAX_QUEUE_NAME];
  while ((buf != ' ') && (current < requestSize))
  {
    buf = request[current];
    queueNameBuf[current++] = buf;
    if (current > MAX_QUEUE_NAME)
    {
      write(2, "too long queue name\n", 21);
      return -1;
    }
  }

  // update buf
  buf = request[current];

  // get rid of extra spaces
  while ((buf == ' ') && (current < requestSize))
  {
    buf = request[++current];
  }

  if (current >= requestSize)
  {
    write(2, "wrong request structure\n", 25);
    return -1;
  }
  queueNameBuf[current - 1] = '\0';

  // read second arg
  char commandBuf[MAX_COMMANDS_SIZE];
  int i = 0;
  if (buf == '"')
  {
    do
    {
      buf = request[++current];
      commandBuf[i++] = buf;
      if (i > MAX_COMMANDS_SIZE)
      {
        write(2, "too long commands\n", 19);
        return -1;
      }
    } while ((buf != '"') && (current < requestSize));
    commandBuf[i - 1] = '\0';
    buf = request[++current];
  }
  else
  {
    while (buf != ' ' && (current < requestSize))
    {
      commandBuf[i++] = buf;
      buf = request[++current];
      if (i > MAX_COMMANDS_SIZE)
      {
        write(2, "too long commands\n", 19);
        return -1;
      }
    }
    commandBuf[i] = '\0';
  }

  // update buf
  buf = request[current];

  // get rid of extra spaces
  while ((buf == ' ') && (current < requestSize))
  {
    buf = request[current++];
  }

  if (current >= requestSize)
  {
    write(2, "wrong request structure\n", 25);
    return -1;
  }

  // read third arg
  char fifoNameBuf[MAX_FIFO_NAME];
  i = 0;
  if (buf == '"')
  {
    do
    {
      buf = request[current++];
      fifoNameBuf[i++] = buf;
      if (i > MAX_FIFO_NAME)
      {
        write(2, "too long fifo name\n", 20);
        return -1;
      }
    } while ((buf != '"' && buf != '\n') && (current < requestSize));
    fifoNameBuf[i - 1] = '\0';
    if (current >= requestSize)
    {
      write(2, "wrong request structure\n", 25);
      return -1;
    }
  }
  else
  {
    while (buf != ' ' && buf != '\n' && (current < requestSize))
    {
      fifoNameBuf[i++] = buf;
      buf = request[current++];
      if (i > MAX_FIFO_NAME)
      {
        write(2, "too long fifo name\n", 20);
        return -1;
      }
    }
    fifoNameBuf[i] = '\0';
  }

  strcpy(requestBuf->queueName, queueNameBuf);
  strcpy(requestBuf->commands, commandBuf);
  strcpy(requestBuf->fifoName, fifoNameBuf);
}

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
    while (1)
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
    while (1)
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