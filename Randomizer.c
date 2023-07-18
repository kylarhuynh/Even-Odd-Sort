#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int i;
    int numint;
    int randint;
    char intstring[50];
    char arg0[10] = "./EvenOdd";
    int fd[2];
    int pid;
    char *arguments[4];

    if (argc < 3 || argc > 3)
    {
        fprintf(stderr, "Invalid number of arguments.\n");
        return 1;
    }

    arguments[0] = arg0;
    arguments[1] = argv[2];
    arguments[2] = arg0;
    arguments[3] = NULL;

    srand(time(NULL));

    numint = atoi(argv[1]);

    if (pipe(fd) == -1)
    { /* create pipe */
        perror("Pipe failed.\n");
        exit(EXIT_FAILURE);
    }

    dup2(fd[0], STDIN_FILENO);

    pid = fork();
    if(pid == 0){
        execv(arg0, arguments); /* extra argument used as flag */
        exit(0);
    }
    else{
    close(fd[0]);
    for (i = 0; i < numint; i++)
    {
        randint = (rand() % (20000 + 1) - 10000);
        sprintf(intstring, "%d ", randint);
        write(fd[1], intstring, sizeof(char) * strlen(intstring));
    }
    write(fd[1], "a", sizeof(char));
    close(fd[1]);
    wait(NULL);
    return 0;
    }
}
