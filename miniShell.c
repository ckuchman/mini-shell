#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>

#define MAX_INPUT 2049
#define MAX_ARG 512

void exitShell(void);
void cd(char *path);
void forkFunc(char *arg[MAX_ARG], char *input, char *output, int background);
void parseCommand(char *userInput, char *arg[MAX_ARG], char **input, char **output, int *background);
void childExec(char *arg[MAX_ARG]);

int main(int argc, char *argv[]) {
    //TODO testing fork limit
    int forkLimit = 20;
    int forkNum = 0;

    //Get memory block for input
    char *userInput;
    userInput = (char*)malloc(sizeof(char) * MAX_INPUT);

    //Storage for parsed input
    char *input;
    char *output;
    char *arg[MAX_ARG];
    int background;

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

            //Parse input
            parseCommand(userInput, arg, &input, &output, &background);

            if (userInput[0] != '#') {
                //Run command
                forkFunc(arg, input, output, background);
            }
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


void forkFunc(char *arg[MAX_ARG], char *input, char *output, int background) {
    pid_t childPid = -5;

    childPid = fork();
    switch (childPid)
    {
        case -1:
            perror("Fork Failure");
            exit(1);
            break;
        case 0:

            if (input != NULL) {

                int inputFile = open(input, O_RDONLY);

                if (inputFile < 0) {
                    printf("Failure to open input file\n");
                }

                if (dup2(inputFile, 0) < 0) {
                    printf("Failed to dup input\n");
                }
            }

            if (output != NULL) {

                int outputFile = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (outputFile < 0) {
                    printf("Failure to open output file\n");
                }

                if (dup2(outputFile, 1) < 0) {
                    printf("Failed to dup output\n");
                }
            }

            childExec(arg);
            exit(1);
            break;
        default:
            //If not a background process then wait for the process
            if (background == 0) {
                childPid = wait(NULL);
            }
            break;
    }
}


void parseCommand(char *userInput, char *arg[MAX_ARG], char **input, char **output, int *background) {
    char *str = NULL;
    char *strNext = NULL;

    //Reset the values from last input
    *input = NULL;
    *output = NULL;
    *background = 0;

    int i;
    for (i = 0; i < MAX_ARG; i++) {
        arg[i] = NULL;
    } 

    //Determines the inputs and stores them
    str = strtok(userInput, " ");
    strNext = strtok(NULL, " ");
  
    int argNum = 0; 
    while (str != NULL) {
        if (strcmp(str, "<") == 0) {
            *input = strtok(NULL, " ");
        } else if (strcmp(str, ">") == 0) {
            *output = strtok(NULL, " ");
        } else if (strcmp(str, "&") == 0 && strNext == NULL) {
            *background = 1;
        } else {
            arg[argNum] = str;
            argNum++;
        }

        //Get next word
        str = strNext;
        strNext = strtok(NULL, " ");
    }
}


void childExec(char *arg[MAX_ARG]) {
    //TODO prints for testing
    //int i = 0;
    //while (arg[i] != NULL) {
    //    printf("Argument %d: %s\n", i, arg[i]);
    //    i++; 
    //}

    execvp(arg[0], arg);
    perror("Exec Failure!\n");
    exit(2);
}
