//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// ficheros por si hay redirección
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Saliendo del MSH **** \n");
	//signal(SIGINT, siginthandler);
        exit(0);
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
    //reset first
    for(int j = 0; j < 8; j++){
        argv_execvp[j] = NULL;
	}
    int i = 0;
    for ( i = 0; argvv[num_command][i] != NULL; i++){
        argv_execvp[i] = argvv[num_command][i];
	
	}
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
    /**** Do not delete this code.****/
    int end = 0; 
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF){
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush (stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;


	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		// Prompt 
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
                //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
                executed_cmd_lines++;
                if( end != 0 && executed_cmd_lines < end) {
                    command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
                }else if( end != 0 && executed_cmd_lines == end)
                    return 0;
                else
                    command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
                //************************************************************************************************


              /************************ STUDENTS CODE ********************************/
	      if (command_counter > 0) {
                if (command_counter > MAX_COMMANDS)
                      printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
                else {
					// Print command
					// print_command(argvv, filev, in_background);
					int pid, exit_code;
					pid = fork();
					switch (pid){
						case -1: /* Check the fork */
							perror("Error while fork.");
							return -1;
						case 0: /* Child process to execute the command */
							getCompleteCommand(argvv, 0); //Get the complete command

							if (in_background){ /* WE CAN DELETE THIS, IS JUST TO CHECK IN BACKGROUND */
								printf("[%d]\n", getpid());
								for(int i = 0; i < 1000000000; i++){
									
								}
							}
							printf("%s %s %s", filev[0], filev[1], filev[2]);
							if (strcmp(filev[0], "0")){ //CHANGE STDIN
								printf("eyy");
								if(close(STDIN_FILENO) < 0){
									perror("Cannot close stdin");
									exit(-1);
								}
								if(open(filev[0], O_RDONLY) < 0){
									perror("Cannot open new stdin");
									exit(-1);
								}
							}
							if (strcmp(filev[1], "0")){ //CHANGE STDOUT
								printf("eyy");
								if(close(STDOUT_FILENO) < 0){
									perror("Cannot close stdout");
									exit(-1);
								}
								if(open(filev[1], O_WRONLY | O_CREAT | O_APPEND, 0660) < 0){
									perror("Cannot open new stdout");
									exit(-1);
								}
							}
							if (strcmp(filev[2], "0")){ //CHANGE STDERR
								if(close(STDERR_FILENO) < 0){
									perror("Cannot close stderr");
									exit(-1);
								}
								if(open(filev[1], O_WRONLY | O_CREAT | O_APPEND, 0660) < 0){
									perror("Cannot open new stderr");
									exit(-1);
								}
							}
							if(execvp(argv_execvp[0], (char* const*) &argv_execvp)){ //Try to execute execvp
								printf("%s",argv_execvp[0]);
								perror("Error in execvp");
								exit(-1);
							}
							exit(0);
						default: /* Parent process */
							if (!in_background){ //If in background, we have to wait
								printf("hola");
								if (waitpid(pid, &exit_code, 0) < 0) {
									perror("cannot waitpid");
									return -1;
								}
							}
					}
                }
              }
        }
	return 0;
}