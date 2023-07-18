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
#include <sys/time.h>
#include <signal.h>

int count;
int processes;

int checksorted(int *);
void sort(int *, int *, int *, int);
void synch(int, int *, int);
void oddsort(int *, int, int);
void evensort(int *, int, int);

int main(int argc, char *argv[])
{
    int *array;
    int *temparray;
    int i, j, scan, integer;
    int *initial;
    int *ready; /* for sync */
    int *sections;
    int pid;
    int *children;
    struct timeval start, end;
    long milliseconds;
    char c;
    char temp[50];

    processes = atoi(argv[1]);

    scan = 0;
    count = 0;
    if (argc == 2)
    {
        while (scan != EOF)
        {
            if (count == 0)
            {
                scan = scanf("%d", &integer);

                if (scan != -1)
                {
                    temparray = (int *)malloc(sizeof(int));
                    temparray[count] = integer;
                    count++;
                }
            }
            else
            {
                scan = scanf("%d", &integer);
                if (scan != -1)
                {
                    temparray = realloc(temparray, count * sizeof(int) + sizeof(int));
                    temparray[count] = integer;
                    count++;
                }
            }
        }
    }
    else if (argc == 3)
    {
        while (read(STDIN_FILENO, &c, sizeof(char)))
        {
            if (c == 'a')
            {
                break;
            }
            if (c == ' ')
            {
                integer = atoi(temp);
                memset(temp, 0, strlen(temp));
                if (count == 0)
                {
                    temparray = (int *)malloc(sizeof(int));
                    temparray[count] = integer;
                    count++;
                }
                else
                {
                    temparray = realloc(temparray, count * sizeof(int) + sizeof(int));
                    temparray[count] = integer;
                    count++;
                }
            }
            else
            {
                strncat(temp, &c, 1);
            }
        }
    }

    initial = temparray;

    if (processes > (count / 2))
    {
        processes = count / 2;
    }

    array = (int *)mmap(NULL, count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    for (i = 0; i < count; i++)
    {
        array[i] = temparray[i];
    }
    ready = (int *)mmap(NULL, processes * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    for (i = 0; i < processes; i++)
    {
        ready[i] = 0;
    }
    sections = (int *)mmap(NULL, processes * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    for (i = 0; i < processes; i++)
    {
        sections[i] = 0;
    }
    j = 0;
    for (i = 0; i < count; i++)
    {
        sections[j] = sections[j] + 1;
        j++;
        if (j == processes)
        {
            j = 0;
        }
    }
    children = mmap(NULL, processes * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    for (i = 0; i < processes; i++)
    {
        children[i] = 0;
    }

    gettimeofday(&start, NULL);

    for (i = 0; i < processes; i++)
    {
        pid = fork();

        if (pid < 0)
        {
            perror("Fork failed.\n");
            return 1;
        }
        else if (pid == 0)
        {
            children[i] = getpid();
            sort(array, ready, sections, i);
            children[i] = 0;
            for (i = 0; i < processes; i++)
            {
                if (children[i] != 0)
                {
                    kill(children[i], SIGKILL);
                }
            }
            exit(0);
        }
    }

    for (i = 0; i < processes; i++)
    { /* wait for all children to finish */
        wait(NULL);
    }

    gettimeofday(&end, NULL);
    milliseconds = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    printf("Initial Array: [");
    for (i = 0; i < count; i++)
    {
        if(i == 0){
            printf("%d,", initial[i]);
        }
        else if (i == count - 1)
        {
            printf(" %d", initial[i]);
        }
        else
        {
            printf(" %d,", initial[i]);
        }
    }
    printf("]\n");

    printf("Sorted Array: [");
    for (i = 0; i < count; i++)
    {
        if(i == 0){
            printf("%d,", array[i]);
        }
        else if (i == count - 1)
        {
            printf(" %d", array[i]);
        }
        else
        {
            printf(" %d,", array[i]);
        }
    }
    printf("]\n\n");
    printf("Processes: %d\n", processes);
    printf("Time to Sort: %li ms\n", milliseconds);

    munmap(ready, processes * sizeof(int));
    munmap(sections, processes * sizeof(int));
    munmap(array, count * sizeof(int));
    free(temparray);

    return 0;
}

void sort(int *array, int *ready, int *sections, int childnum)
{
    int i;
    int start;
    int end;

    start = 0;
    end = 0;
    for (i = 0; i < childnum; i++)
    {
        start = start + sections[i];
    }
    end = start + sections[childnum];

    i = 0;
    while (checksorted(array) != 0)
    {
        i++;
        synch(childnum, ready, i);
        oddsort(array, start, end);
        i++;
        synch(childnum, ready, i);
        evensort(array, start, end);
        /*for (int j = 0; j < count; j++)
        {
            fprintf(stderr, "%d ", array[j]);
        }
        fprintf(stderr, "\n");*/
    }
    return;
}

void oddsort(int *array, int start, int end)
{
    int i;
    int temp;
    for (i = start; i < end; i++)
    {
        if (i % 2 == 0 && (i + 1 < count))
        {
            if (array[i] > array[i + 1])
            {
                temp = array[i];
                array[i] = array[i + 1];
                array[i + 1] = temp;
            }
        }
    }
    return;
}

void evensort(int *array, int start, int end)
{
    int i;
    int temp;

    for (i = start; i < (end); i++)
    {
        if (i % 2 != 0 && (i + 1 < count))
        {
            if (array[i] > array[i + 1])
            {
                temp = array[i];
                array[i] = array[i + 1];
                array[i + 1] = temp;
            }
        }
    }
    return;
}

void synch(int childnum, int *ready, int level)
{
    int i;
    int breakout = 0;

    ready[childnum] = ready[childnum] + 1;

    while (breakout == 0)
    {
        breakout = 1;
        for (i = 0; i < processes; i++)
        {
            if (ready[i] < level)
            {
                breakout = 0;
            }
        }
    }
    /* fprintf(stderr, "Par_id: %d level: %d\n", childnum, ready[childnum]); */
}

int checksorted(int *array)
{
    int i;

    for (i = 0; i < (count - 1); i++)
    {
        if (array[i] > array[i + 1])
        {
            return 1;
        }
    }
    return 0;
}
