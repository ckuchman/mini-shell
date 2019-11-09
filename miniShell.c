#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void exitShell(void);
void cd(char *path);
void execFunc(char *userInput);
void childEndless(char *input);

#define MAX_INPUT 2049

int main(int argc, char *argv[]) {
    //TODO testing fork limit
    int forkLimit = 2;
    int forkNum = 0;

    //Get memory block for input
    char *userInput;
    userInput = (char*)malloc(sizeof(char) * MAX_INPUT);

    //Main shell loop
    while (strcmp(userInput, "exit") != 0) {

        //Take in user input
        printf(": ");
        fgets(userInput, MAX_INPUT, stdin);

        //Clears new line
        if (userInput[strlen(userInput) - 1] == '\n') {
            userInput[strlen(userInput) - 1] = '\0';
        }

        if (strcmp(userInput, "cd") == 0) {
            cd("");
        } else if (strncmp(userInput, "cd ", 3) == 0) {
            cd(userInput + 3);
        } else if (strcmp(userInput, "status") == 0) {
            //TODO status function
        } else {
            forkNum++;
            printf("Fork: %d PID: %d\n", forkNum, getpid());
            execFunc(userInput);
        }

        //TODO testing fork bomb protection
        if (forkNum > forkLimit) {
            abort();
        }
    }

    // Take care of children processes
    exitShell();

    //Clear memory
    free(userInput);

    return 0;
}


void exitShell(void) {
    //TODO Kill all other processes currently running started by the shell
    printf("exit run");
}


void cd(char *path) {
    char *truePath;
    truePath = (char*)malloc(sizeof(char) * MAX_INPUT);

    //If path is empty cd to HOME otherwise go to location
    if (strcmp(path, "") == 0) {
        printf("no input, using HOME");

        truePath = getenv("HOME");
    } else {
        truePath = path;
    }

    chdir(truePath);
}


void execFunc(char *userInput) {
    //TODO Temp child fork
    pid_t childPid = -5;

    childPid = fork();
    switch (childPid)
    {
        case -1:
            perror("Fork Failure");
            exit(1);
            break;
        case 0:
            childEndless(userInput);
            printf("child");
            exit(1);
            break;
        default:
            break;
    }
}


void childEndless(char *input) {
    while (1) {
        printf("%s\n", input);
        sleep(3);
    }
}
