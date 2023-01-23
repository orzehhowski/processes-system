# processes-system
student project - Processes communication system in C

## how it works
Compile program:

    gcc main.c -o main

In config file you can find processes names and assigned keys.
By running program with process name passed as argument:

    ./main processName
    
You will run message queue with key connected to choosen name. Then you can run commands with given syntax (apostrophes are required only for commands containing whitespaces):

    secondProcessName "commands to do" fifoName
   
Process will receive this command, get key of given second process from config file and send to its message queue request containing name of created FIFO. Then it send commands to do (for example `ls -a | wc` through this FIFO. Second process will receive commands, execute them and put results inside previously mentioned FIFO. First process will read results and print them. 

Every process have got two synchronous threads - one to listen to users input, and second to recieve requests from other processes. Both of them will keep you updated by reporting their status.

## warning

Program may not work on Windows/MacOS.
