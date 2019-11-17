#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

#define MAX_INPUT 2049
#define MAX_ARG 512
#define MAX_THREADS 100


void cd(char *path);
pid_t forkFunc(char *arg[MAX_ARG], char *input, char *output, int background, int *childExitMethod);
void expandString(char *str, int strLen);
void parseCommand(char *userInput, char *arg[MAX_ARG], char **input, char **output, int *background);

//TODO List
//should look at PATH for commands?
//Print the process of a background process when it begins
//Print a message of background process and exit status when it ends
//Implement signals

int main(int argc, char *argv[]) {
    //Child tracker
    int childCount = 0;
    pid_t childVal;
    int fgExitMethod = -10; 
    int childExitMethod, exitStatus;

    pid_t *childs = (pid_t*)malloc(sizeof(pid_t) * MAX_THREADS);
    
    int i = 0;
    for (i = 0; i < MAX_THREADS; i++) {
        childs[i] = -10;
    }

    //Get memory block for input
    char *userInput;
    userInput = (char*)malloc(sizeof(char) * MAX_INPUT);

    //Storage for parsed input
    char *input;
    char *output;
    char *arg[MAX_ARG];
    int background;


    pid_t childPid;

    //Main shell loop
    while (strcmp(userInput, "exit") != 0) {

        //Take in user input
        printf(": ");
        fgets(userInput, MAX_INPUT, stdin);

        //Clears new line
        if (userInput[strlen(userInput) - 1] == '\n') {
            userInput[strlen(userInput) - 1] = '\0';
        }

        expandString(userInput, MAX_INPUT);

        if (strcmp(userInput, "cd") == 0) {
            cd("");
        } else if (strncmp(userInput, "cd ", 3) == 0) {
            cd(userInput + 3);
        } else if (strcmp(userInput, "status") == 0) {
            if (fgExitMethod == -10) {
                printf("exit status 0?\n");
            } else {

                if (WIFEXITED(fgExitMethod)) {
                    exitStatus = WEXITSTATUS(fgExitMethod);
                    printf("exit status %d\n", exitStatus);
                }

                if (WIFSIGNALED(fgExitMethod)) {
                    exitStatus = WTERMSIG(fgExitMethod);
                    printf("terminated by signal %d\n", exitStatus);
                }

            }
        } else if (strcmp(userInput, "exit") != 0) {
           //printf("Fork: %d PID: %d\n", forkNum, getpid());

            //Parse input
            parseCommand(userInput, arg, &input, &output, &background);

            if (userInput[0] != '#') {
                //Run command
                if (childCount < MAX_THREADS) {
                    childPid = forkFunc(arg, input, output, background, &fgExitMethod);

                    if (childPid > 0) {

                        childCount++;

                        //Store the child pid in the child array
                        i = 0;
                        while (childs[i] != -10) {
                            i++;
                        }
                        
                        //printf("Added %d to %d\n", childPid, i);
                        childs[i] = childPid;
                    }                   
                } else {
                    printf("Too many children, wait for processes to end\n");
                }
            }
        }

        //Check the children
        for (i = 0; i < MAX_THREADS; i++) {
            if (childs[i] != -10) {
                childVal = waitpid(childs[i], &childExitMethod, WNOHANG); 

                if (childVal != 0) {
                    childs[i] = -10;
                    //printf("Removed %d from %d\n", childPid, i);
                }
            }        
        }
    }

    //Exit Processes
    for (i = 0; i < MAX_THREADS; i++) {
        if (childs[i] != -10) { 
            kill(childs[i], SIGINT);
        }        
    }   

    //Clear memory
    free(childs);
    free(userInput);

    return 0;
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


pid_t forkFunc(char *arg[MAX_ARG], char *input, char *output, int background, int *childExitMethod) {
    pid_t childPid = -5;

    childPid = fork();
    switch (childPid)
    {
        case -1:
            perror("Fork Failure");
            exit(1);
            break;
        case 0:
            if (input != NULL || background != 0) {

                if (input == NULL) {
                    input = "/dev/null";
                }

                int inputFile = open(input, O_RDONLY);

                if (inputFile < 0) {
                    printf("Failure to open input file\n");
                    exit(1);
                }

                if (dup2(inputFile, 0) < 0) {
                    printf("Failed to dup input\n");
                    exit(1);
                }

                close(inputFile);
            }

            if (output != NULL || background != 0) {

                if (output == NULL) {
                    output = "/dev/null";
                }

                int outputFile = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (outputFile < 0) {
                    printf("Failure to open output file\n");
                    exit(1);
                }

                if (dup2(outputFile, 1) < 0) {
                    printf("Failed to dup output\n");
                    exit(1);
                }

                close(outputFile);
            }

            //TODO prints for testing
            //int i = 0;
            //while (arg[i] != NULL) {
            //    printf("Argument %d: %s\n", i, arg[i]);
            //    i++; 
            //}

            execvp(arg[0], arg);
            perror("Exec Failure!\n");
            exit(1);
            break;
        default:
            //If not a background process then wait for the process
            if (background == 0) {
                childPid = waitpid(childPid, childExitMethod, 0);
            }
            break;
    }

    return childPid;
}


void expandString(char *str, int strLen) {
    int pid = getpid();

    int pidLen = snprintf(NULL, 0, "%d", pid);

    char* pidStr = malloc(pidLen + 1);
    snprintf(pidStr, pidLen + 1, "%d", pid);

    char* tempStr = (char*)malloc(sizeof(char) * MAX_INPUT);
    strcpy(tempStr, str);    

    int i = 0, j = 0, k = 0;
    while (tempStr[i] != '\0') {

        if (tempStr[i] == '$' && tempStr[i+1] == '$') {
            while (pidStr[k] != '\0') {
                str[j] = pidStr[k];
                j++;
                k++;
            }

            k = 0;
            i += 2;
        } else {
            str[j] = tempStr[i];

            i++;
            j++;
        }
    }

    free(pidStr);
    free(tempStr);
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
            *input = strNext;

            str = strtok(NULL, " ");
            
            if (str != NULL) {
                strNext = strtok(NULL, " ");
            } else {
                strNext = NULL;
            }
        } else if (strcmp(str, ">") == 0) {
            *output = strNext;

            str = strtok(NULL, " ");
            
            if (str != NULL) {
                strNext = strtok(NULL, " ");
            } else {
                strNext = NULL;
            }

        } else if (strcmp(str, "&") == 0 && strNext == NULL) {
            *background = 1;

            str = strNext;
        } else {
            arg[argNum] = str;
            argNum++;

            str = strNext;
            strNext = strtok(NULL, " ");
        }
    }
}
