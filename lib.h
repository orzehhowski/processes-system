#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#define MAX_QUEUE_NAME 64
#define MAX_KEY_STR 16
#define MAX_REQUEST 192
#define MAX_COMMANDS_SIZE 64
#define MAX_FIFO_NAME 64
#define MAX_RESPONSE 1024

struct msgbuf
{
  long mtype;
  char fifoName[MAX_FIFO_NAME];
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
    return -1;
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
      return -1;
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
        return -1;
      }
      close(configFd);
      return atoi(keyBuf);
    }

    // else go to next line
    char buf = 0;
    while (buf != '\n' && readValue > 0)
    {
      readValue = read(configFd, &buf, 1);
    }
  }
  // if couldn't find matching name
  close(configFd);
  write(2, "couldn't find matching queue name in config file\n", 50);
  return -1;
}

// returns ID of created queue
int createProcessQueue(int key)

{
  int QueueId = msgget(key, IPC_CREAT | 0640);
  if (QueueId == -1)
  {
    perror("Create process queue error");
    return -1;
  }
  return QueueId;
}

int waitForMessages(int queueId, struct msgbuf *messageBuf)
{
  int received = msgrcv(queueId, messageBuf, MSG_SIZE, 0, 0);
  if (received == -1)
  {
    perror("receiving message error");
    return -1;
  }
  return received;
}

int splitRequest(char requestText[], struct request *requestBody)
{
  int requestSize = strlen(requestText);
  int current = 0;
  char buf;

  // read first arg
  char queueNameBuf[MAX_QUEUE_NAME];
  while ((buf != ' ') && (current < requestSize))
  {
    buf = requestText[current];
    queueNameBuf[current++] = buf;
    if (current > MAX_QUEUE_NAME)
    {
      write(2, "too long queue name\n", 21);
      return -1;
    }
  }

  // update buf
  buf = requestText[current];

  // get rid of extra spaces
  while ((buf == ' ') && (current < requestSize))
  {
    buf = requestText[++current];
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
      buf = requestText[++current];
      commandBuf[i++] = buf;
      if (i > MAX_COMMANDS_SIZE)
      {
        write(2, "too long commands\n", 19);
        return -1;
      }
    } while ((buf != '"') && (current < requestSize));
    commandBuf[i - 1] = '\0';
    buf = requestText[++current];
  }
  else
  {
    while (buf != ' ' && (current < requestSize))
    {
      commandBuf[i++] = buf;
      buf = requestText[++current];
      if (i > MAX_COMMANDS_SIZE)
      {
        write(2, "too long commands\n", 19);
        return -1;
      }
    }
    commandBuf[i] = '\0';
  }

  // update buf
  buf = requestText[current];

  // get rid of extra spaces
  while ((buf == ' ') && (current < requestSize))
  {
    buf = requestText[current++];
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
      buf = requestText[current++];
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
      buf = requestText[current++];
      if (i > MAX_FIFO_NAME)
      {
        write(2, "too long fifo name\n", 20);
        return -1;
      }
    }
    fifoNameBuf[i] = '\0';
  }

  strcpy(requestBody->queueName, queueNameBuf);
  strcpy(requestBody->commands, commandBuf);
  strcpy(requestBody->fifoName, fifoNameBuf);
  return 0;
}

int sendFifoName(char fifoName[], char queueName[])
{
  int processKey = readKeyFromConfig(queueName);
  if (processKey == -1)
  {
    return -1;
  }
  int queueId = msgget(processKey, IPC_CREAT | 0640);
  if (queueId == -1)
  {
    perror("get queue error");
    return -1;
  }

  struct msgbuf message;
  strcpy(message.fifoName, fifoName);
  message.mtype = 1;

  int wasSend = msgsnd(queueId, &message, MSG_SIZE, IPC_NOWAIT);
  if (wasSend == -1)
  {
    perror("send message error");
    return -1;
  }

  return 0;
}

int readFromFifo(char fifoName[], char messageBuf[], int maxSize)
{
  int deskToRead = open(fifoName, O_RDONLY);
  if (deskToRead == -1)
  {
    perror("open fifo error");
    return -1;
  }

  int readRes = read(deskToRead, messageBuf, maxSize);
  if (readRes == -1)
  {
    close(deskToRead);
    perror("read from fifo error");
    return -1;
  }
  close(deskToRead);
  return 0;
}

int splitBy(char toSplit[], char *mainBuf[], char splitter)
{
  int size = strlen(toSplit);
  int current = 0;
  int currentWord = 0;
  while (current < size)
  {
    char buf = toSplit[current];
    char wordBuf[64];
    int i = 0;
    while (buf != splitter && current < size)
    {
      wordBuf[i++] = buf;
      buf = toSplit[++current];
    }
    wordBuf[i] = '\0';
    while (buf == splitter && current < size)
    {
      buf = toSplit[++current];
    }
    if (i < 1)
    {
      continue;
    }
    mainBuf[currentWord] = (char *)malloc(16);
    strcpy(mainBuf[currentWord++], wordBuf);
  }
  return currentWord;
}

int executeCommands(char commands[], char fifoName[])
{
  char *commandsArray[16];
  char results[MAX_RESPONSE];

  int commandsNum = splitBy(commands, commandsArray, '|');
  commandsArray[commandsNum] = NULL;

  int pdesks[2 * commandsNum];
  int i;
  for (i = 0; i < commandsNum; i++)
  {
    if (pipe(pdesks + i * 2) == -1)
    {
      perror("create pipe error");
      return -1;
    }
  }
  int resultDesk[2];
  if (pipe(resultDesk) == -1)
  {
    perror("create pipe error");
    return -1;
  }

  int currentIndex = 0;
  while (commandsArray[currentIndex] != NULL)
  {

    printf("[1] executing command %s\n", commandsArray[currentIndex]);

    char *execVector[16];
    int vectorLen = splitBy(commandsArray[currentIndex], execVector, ' ');
    execVector[vectorLen] = NULL;

    if (!fork())
    {

      // if not first command
      if (currentIndex != 0)
      {
        if (dup2(pdesks[(currentIndex - 1) * 2], 0) == -1)
        {
          perror("dup error");
          return -1;
        }
      }

      // if not last command
      if (currentIndex != commandsNum - 1)
      {
        if (dup2(pdesks[currentIndex * 2 + 1], 1) == -1)
        {
          perror("dup error");
          return -1;
        }
      }

      // if last command
      if (currentIndex == commandsNum - 1)
      {
        // CZEMU TO NIE DZIALAAAAAAAA
        // dup2(resultDesk[1], 1);

        // save to FIFO

        printf("[1] saving result to fifo %s\n", fifoName);

        int fifoDs = open(fifoName, O_WRONLY);
        if (fifoDs == -1)
        {
          perror("open fifo error");
          return -1;
        }

        if (dup2(fifoDs, 1) == -1)
        {
          perror("dup error");
          return -1;
        }
      }

      int i;
      for (i = 0; i < commandsNum * 2; i++)
      {
        close(pdesks[i]);
      }

      execvp(execVector[0], execVector);
    }

    currentIndex++;
  }

  for (i = 0; i < commandsNum * 2; i++)
  {
    close(pdesks[i]);
  }

  return 0;
}