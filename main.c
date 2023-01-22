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

int readKeyFromConfig(char *argv[])
{

  int configFd = open("./config.txt", O_RDONLY);
  if (configFd == -1)
  {
    perror("opening config file error");
    exit(1);
  }

  char queueName[MAX_QUEUE_NAME];
  strcpy(queueName, argv[1]);

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

int main(int argc, char *argv[])
{
  checkInit(argc, argv);

  int queueKey = readKeyFromConfig(argv);

  printf("%d\n", queueKey);
}