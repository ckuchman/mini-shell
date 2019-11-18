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


//TODO make into a sig_atomic_t
int suppressBck = 0;


pid_t forkFunc(char *arg[MAX_ARG], char *input, char *output, int background, int *childExitMethod);
void expandString(char *str, int strLen);
void parseCommand(char *userInput, char *arg[MAX_ARG], char **input, char **output, int *background);
void suppressBackground(int sig);


//TODO List
//should look at PATH for commands?


int main(int argc, char *argv[]) {

    //Creating the favorite iterator
    int i;

    //Child tracker variables
    pid_t childPid;
    int childCount = 0;
    int fgExitMethod = -10; 
    int childExitMethod = -10;
    int exitStatus;

    //Generate array to store child process id's, using -10 as a null indicator
    pid_t *childs = (pid_t*)malloc(sizeof(pid_t) * MAX_THREADS);
    
    for (i = 0; i < MAX_THREADS; i++) {
        childs[i] = -10;
    }

    //Sets parent shell to ignore Ctrl+C commands
    struct sigaction ign = {0};
    ign.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ign, NULL);

    //Sets parent shell to toggle suppression of background on Ctrl+Z
    struct sigaction supBck = {0};
    supBck.sa_handler = suppressBackground;
    sigaction(SIGTSTP, &supBck, NULL);

    //Get memory block for input
    char *userInput;
    userInput = (char*)malloc(sizeof(char) * MAX_INPUT);

    //Storage for parsed input
    char *input;
    char *output;
    char *arg[MAX_ARG];
    int background;

    //Main shell loop which ends on exit command
    while (strcmp(arg[0], "exit") != 0) {

        //Take in user input
        printf(": ");
        fflush(stdout);
        fgets(userInput, MAX_INPUT, stdin); 

        //Clears new line from input
        if (userInput[strlen(userInput) - 1] == '\n') {
            userInput[strlen(userInput) - 1] = '\0';
        }

        //Replaces $$ with shell process id
        expandString(userInput, MAX_INPUT);

        //Parse input
        parseCommand(userInput, arg, &input, &output, &background);

        //If the suppress background setting is active then treat as
        //always not run on background
        if (suppressBck != 0) {
            background = 0;
        }

        //Check over built in commands
        if (strcmp(arg[0], "cd") == 0) {

	    //If path is empty cd to HOME otherwise go to location
	    if (arg[1] == NULL) {
		chdir(getenv("HOME"));
	    } else {
		chdir(path);
	    }

        } else if (strcmp(arg[0], "status") == 0) {

            //Checks if a foreground process has been ran
            if (fgExitMethod == -10) {
                printf("exit status 0\n");
                fflush(stdout);
            } else {

                //Prints the appropriate message dependent on if 
                //it was a exit status or signal
                if (WIFEXITED(fgExitMethod)) {
                    exitStatus = WEXITSTATUS(fgExitMethod);
                    printf("exit status %d\n", exitStatus);
                    fflush(stdout);
                }
                if (WIFSIGNALED(fgExitMethod)) {
                    exitStatus = WTERMSIG(fgExitMethod);
                    printf("terminated by signal %d\n", exitStatus);
                    fflush(stdout);
                }
            }
        } else if (strcmp(arg[0], "exit") != 0 && arg[0] != "" && userInput[0] != '#') {

	    //Run command so long as there are not too many background processes being run
	    if (childCount < MAX_THREADS) {

                //Fork child to run command
		childPid = forkFunc(arg, input, output, background, &fgExitMethod);

                //If the child was successfully created and ran in the background
		if (childPid > 0 && background != 0) {

		    //Store the child pid in the child array
		    i = 0;
		    while (childs[i] != -10) {
			i++;
		    }		    
		    childs[i] = childPid;
                    childCount++;
		}                   
	    } else {

                //Error if too many things in background
		printf("Too many children, wait for processes to end\n");
		fflush(stdout);
	    }
        }

        //Check the background children for zombies
        for (i = 0; i < MAX_THREADS; i++) {

            //If the array is storing a process number then check on it
            if (childs[i] != -10) {

                //Check individual child status
                childPid = waitpid(childs[i], &childExitMethod, WNOHANG); 

                //If the child has already terminated then display exit status/term signal, otherwise ignore
                if (childPid != 0) {

                    //Prints the appropriate message dependent on if 
                    //it was a exit status or signal
                    if (WIFEXITED(childExitMethod)) {
                        exitStatus = WEXITSTATUS(childExitMethod);
                        printf("Process %d has ended, exit status %d\n", childPid, exitStatus);
                        fflush(stdout);
                    }
                    if (WIFSIGNALED(childExitMethod)) {
                        exitStatus = WTERMSIG(childExitMethod);
                        printf("Process %d has ended, terminated by signal %d\n", childPid, exitStatus);
                        fflush(stdout);
                    }

                    //Remove the process from the child array
                    childs[i] = -10;
                    childCount--;
                }
            }        
        }
    }

    //Exit all background processes
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


pid_t forkFunc(char *arg[MAX_ARG], char *input, char *output, int background, int *childExitMethod) {

    pid_t childPid = -10;

    childPid = fork();
    switch (childPid)
    {
        case -1:
            perror("Fork Failure");
            fflush(stderr);
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
                    fflush(stdout);
                    exit(1);
                }

                if (dup2(inputFile, 0) < 0) {
                    printf("Failed to dup input\n");
                    fflush(stdout);
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
                    fflush(stdout);
                    exit(1);
                }

                if (dup2(outputFile, 1) < 0) {
                    printf("Failed to dup output\n");
                    fflush(stdout);
                    exit(1);
                }

                close(outputFile);
            }


            if (background == 0) {
                //Handle shell signals
                struct sigaction dft = {0};
                dft.sa_handler = SIG_DFL;
                sigaction(SIGINT, &dft, NULL);
            }

            //Handle shell signals
            struct sigaction stp = {0};
            stp.sa_handler = SIG_IGN;
            sigaction(SIGTSTP, &stp, NULL); 

            execvp(arg[0], arg);
            perror("Exec Failure!\n");
            fflush(stderr);
            exit(1);
            break;
        default:
            //If not a background process then wait for the process
            if (background == 0) {

                childPid = waitpid(childPid, childExitMethod, 0);

                if (WIFSIGNALED(*childExitMethod)) {
                    int exitStatus = WTERMSIG(*childExitMethod);
                    printf("terminated by signal %d\n", exitStatus);
                    fflush(stdout);
                }
            } else {
                printf("Background process %d started\n", childPid);
                fflush(stdout);
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
    str[j] = '\0';

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

    if (*userInput == '\0') {
         arg[0] = "";
         return;
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

void suppressBackground(int sig) {
    if (suppressBck == 1) {
        char *message = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        fflush(stdout);
        suppressBck = 0;
    } else {
        char *message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        fflush(stdout);
        suppressBck = 1;
    }
}
