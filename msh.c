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
#include <errno.h>

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

int mycalc(char **argv){
	int num1, num2, res;
	if (argv[4] != NULL || argv[3] == NULL || argv[2] == NULL || argv[1] == NULL){ //Check if the input has the correct number of arguments
		return -1;
	}
	num1 = atoi(argv[1]); //Convert argument 1 into the number
	num2 = atoi(argv[3]); //Convert argument 2 into the number
	if ((num1 == 0 && strcmp(argv[1], "0") != 0) || (num2 == 0 && strcmp(argv[3], "0") != 0)){ //Check if num1 or num2 are valid numbers
		return -1;
	}
	if (strcmp(argv[2], "add") == 0){ //Add operation
		char *acc;
		int newacc;
		acc = getenv("Acc"); //Retrieve the value of Acc
		res = num1 + num2;
		newacc = atoi(acc); //Convert into int
		newacc += res; //Update the value of Acc
		sprintf(acc, "%d", newacc); //Set Acc again to str
		if (setenv("Acc", acc, 1)){ //Try to overwrite the value of Acc
			perror("Error overwrite the envirnoment variable for mycalc");
		}
		fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", num1, num2, res, newacc); //Print in stderr the result of the operation
		return 0;
	} else if (strcmp(argv[2], "mod") == 0){ //Mod operation
		int quot;
		quot = num1 / num2; //Get quotient (division of ints)
		res = num1 % num2; //Get reminder
		fprintf(stderr, "[OK] %d %% %d = %d * %d + %d\n", num1, num2, num2, quot, res); //Print in stderr the result of the operation
		return 0;
	} else { //Argv[2] is not add nor mod 
		return -1;
	}
}

void mycp(char **argv){
	int fd1, fd2, writecount, readcount;
	char buffer[512];
	if (argv[3] != NULL || argv[2] == NULL || argv[1] == NULL){ //Check if the input has the correct number of arguments
		fprintf(stdout, "[ERROR] The structure of the command is mycp <original file> <copied file>\n");
		return;
	}
	if ((fd1 = open(argv[1], O_RDONLY, 0660)) < 0){ //Open the original file
		fprintf(stdout, "[ERROR] Error opening original file: %s\n", strerror(errno));
		return;
	}
	if ((fd2 = open(argv[2], O_WRONLY | O_CREAT, 0660)) < 0){ //Open the destination file
		fprintf(stdout, "[ERROR] Error opening destination file: %s\n", strerror(errno));
		return;
	}
  
	while ((readcount = read(fd1,buffer,512)) > 0){ //Loop to read the copied file until finish or error
		writecount = write(fd2,buffer,readcount);  //Write in the output file
		if (writecount < 0){ //If error while writing, print error and close all files
			fprintf(stdout, "[ERROR] %s\n", strerror(errno));
			if(close(fd1) < 0){
				fprintf(stdout, "[ERROR] %s\n", strerror(errno));
				return;
			}
			if(close(fd2) < 0){
				fprintf(stdout, "[ERROR] %s\n", strerror(errno));
				return;
			}
			return;
		}
	}
	if (readcount < 0){ //Check if there has been an error while reading. If so, print error and close all files
			fprintf(stdout, "[ERROR] %s\n", strerror(errno));
			if(close(fd1) < 0){ 
				fprintf(stdout, "[ERROR] %s\n", strerror(errno));
				return;
			}
			if(close(fd2) < 0){
				fprintf(stdout, "[ERROR] %s\n", strerror(errno));
				return;
			}
			return;
	}
	if(close(fd1) < 0){ //Close original file
		fprintf(stdout, "[ERROR] %s\n", strerror(errno));
		return;
	}
	if(close(fd2) < 0){ //close destination file
		fprintf(stdout, "[ERROR] %s\n", strerror(errno));
		return;
	}
	fprintf(stdout, "[OK] Copy has been successful between %s and %s\n",argv[1],argv[2]);
	return;
}

int modifyError(void){
	/*FUNCTION TO REDIRECT ERROR STREAM*/
	int fd, dupfd;
	if((fd = open(filev[2], O_WRONLY | O_CREAT, 0660)) < 0){ //Open new STDERR file
		fprintf(stderr, "Cannot open stderr %s: %s\n", filev[2], strerror(errno));
		return -1;
	}
	if(close(STDERR_FILENO) < 0){ //Close current STDERR
		perror("Cannot close stderr.");
		return -1;
	}
	if((dupfd = dup(fd)) < 0){ //Duplicate fd to set it to STDERR
		perror("Error duplicating stderr.");
		return -1;
	}
	if(close(fd) < 0){ //Close STDERR file
		perror("Error closing new stderr.");
		return -1;
	}
	return 0;
}

int modifyInput(void){
	/*FUNCTION TO REDIRECT INPUT STREAM*/
	int fd, dupfd;
	if((fd = open(filev[0], O_RDONLY)) < 0){ //Open new STDIN file
		fprintf(stderr, "Cannot open stdin %s: %s\n", filev[0], strerror(errno));
		return -1;
	}
	if(close(STDIN_FILENO) < 0){ //Close current STDIN
		perror("Cannot close stdin.");
		return -1;
	}
	if((dupfd = dup(fd)) < 0){ //Duplicate fd to set it to STDIN
		perror("Error redirecting stdin.");
		return -1;
	}
	if(close(fd) < 0){ //Close STDIN file
		perror("Error closing new stdin.");
		return -1;
	}
	return 0;
}

int modifyOutput(void){
	/*FUNCTION TO REDIRECT OUTPUT STREAM*/
	int fd, dupfd;
	if((fd = open(filev[1], O_WRONLY | O_CREAT, 0660)) < 0){ //Open new STDIN file
		fprintf(stderr, "Cannot open stdout %s: %s\n", filev[1], strerror(errno));
		return -1;
	}
	if(close(STDOUT_FILENO) < 0){ //Close current STDOUT
		perror("Cannot close stdout");
		return -1;
	}
	if((dupfd = dup(fd)) < 0){ //Duplicate fd to set it to STDOUT
		perror("Error redirecting stdout");
		return -1;
	}
	if(close(fd) < 0){ //Close new STDOUT file
		perror("Error closing new stdout");
		return -1;
	}
	return 0;
}

void handle_sigchld(int sig) { //Function to handle Zombies
  int saved_errno = errno;
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
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
	if (setenv("Acc", "0", 0)){ //Initialize the environment variable for mycalc
		perror("Error setting the envirnoment variable for mycalc.");
		return -1;
	}


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
					if (strcmp(argvv[0][0], "mycalc") == 0){
						if (mycalc(argvv[0])){
							printf("[ERROR] The structure of the command is <operand 1> <add/mod> <operand 2>\n");
						}
					} else if (strcmp(argvv[0][0], "mycp") == 0){
						mycp(argvv[0]);
					} else {
						errno = 0;
						int pid, exit_code;
						pid = fork(); //Fork for new process
						switch (pid){
							case -1: /* Check if there has been an error in the fork */
								perror("Error while fork.");
								exit(-1);
							case 0: /* Child process to execute the command */
								
								if (command_counter == 1){
									if (strcmp(filev[2], "0")){ //Change STDERR if necessary
										if(modifyError()){
											exit(-1);
										}
									}
									if (strcmp(filev[0], "0")){ //Change STDIN if necessary
										if(modifyInput()){
											exit(-1);
										}
									}
									if (strcmp(filev[1], "0")){ //Change STDOUT if necessary
										if(modifyOutput()){
											exit(-1);
										}
									}
									if (in_background){ /* Print PID to execute the command in background */
									printf("[%d]\n", getpid());
									}
									getCompleteCommand(argvv, 0); //Get the complete command
									if(execvp(argv_execvp[0], (char* const*) &argv_execvp)){ //Try to execute execvp
										fprintf(stderr, "Error executing %s: %s\n", argv_execvp[0], strerror(errno)); //If not executed, print in stderr
										exit(-1);
									}
								} else if (command_counter > 1){
									if (strcmp(filev[2], "0")){ //Change STDERR if necessary
										if(modifyError()){
											exit(-1);
										}
									}
									int p[command_counter - 1][2]; //Create the matrix of pipes. We need one for every two commands
									int cpid, exit_code2;
									for(int i = 0; i < command_counter; i++){
										if (i != command_counter - 1){
											pipe(p[i]); //Initialize a pipe to communicate this child with the next one
										}
										cpid = fork();
										switch (cpid){
											case -1:
												perror("Error forking child");
												exit(-1);
											case 0:
												getCompleteCommand(argvv, i); //Get the complete command
												if (i == 0){ //If it's the first command, we only need to redirect stdout to the pipe
													if(close(p[0][STDIN_FILENO]) < 0){ //Close unused part of the pipe
														fprintf(stderr, "Error closing read for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (close(STDOUT_FILENO) < 0){ //Close STDOUT
														fprintf(stderr, "Error closing STDOUT to pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (dup(p[0][STDOUT_FILENO]) < 0){ //Redirect STDOUT to pipe
														fprintf(stderr, "Error redirecting STDOUT to pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (close(p[0][STDOUT_FILENO]) < 0){ //Close pipe in file table
														fprintf(stderr, "Error closing read for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (strcmp(filev[0], "0")){ //Change STDIN if necessary
														if(modifyInput()){
															exit(-1);
														}
													}
												} else if (i == command_counter - 1) {
													if (in_background){ /* Print PID to execute the command in background */
														printf("[%d]\n", getpid());
													}
													if (close(p[command_counter - 2][STDOUT_FILENO]) < 0){ //Close unused part of the pipe
														fprintf(stderr, "Error closing write for pipe %d: %s", i-1, strerror(errno));
														exit(-1);
													}
													if (close(STDIN_FILENO) < 0){ //Close STDIN
														fprintf(stderr, "Error closing STDIN for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (dup(p[command_counter - 2][STDIN_FILENO]) < 0){ //Redirect STDIN to pipe
														fprintf(stderr, "Error redirecting STDIN from pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (close(p[command_counter - 2][STDIN_FILENO]) < 0){ //Close pipe in file table
														fprintf(stderr, "Error closing read for pipe %d: %s", i-1, strerror(errno));
														exit(-1);
													}
													if (strcmp(filev[1], "0")){ //Change STDOUT if necessary
														if(modifyOutput()){
															exit(-1);
														}
													}
												} else { //Here we have  STDIN = p[i-1][0] (output of previous command) and STDOUT = p[i][1] (output of this command)
													if (close(p[i-1][STDOUT_FILENO]) < 0){ //Close unused part of the pipe
														fprintf(stderr, "Error closing write for pipe %d: %s", i-1, strerror(errno));
														exit(-1);
													}
													if(close(STDIN_FILENO) < 0){ //Close STDIN
														fprintf(stderr, "Error closing STDIN for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (dup(p[i-1][STDIN_FILENO]) < 0){ //Redirect STDIN to pipe
														fprintf(stderr, "Error redirecting STDIN to pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (close(p[i-1][STDIN_FILENO]) < 0){ //Close pipe in file table
														fprintf(stderr, "Error closing read for pipe %d: %s", i-1, strerror(errno));
														exit(-1);
													}
													if (close(p[i][STDIN_FILENO]) < 0){ //Close unused part of the pipe
														fprintf(stderr, "Error closing read for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (close(STDOUT_FILENO) < 0){ //Close STDOUT
														fprintf(stderr, "Error closing STDOUT for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (dup(p[i][STDOUT_FILENO]) < 0){ //Close pipe in file table
														fprintf(stderr, "Error redirecting write to pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
													if (close(p[i][STDOUT_FILENO]) < 0){
														fprintf(stderr, "Error closing write for pipe %d: %s", i, strerror(errno));
														exit(-1);
													}
												}
												
												if(execvp(argv_execvp[0], (char* const*) &argv_execvp)){ //Try to execute execvp
													fprintf(stderr, "Error executing %s: %s\n", argv_execvp[0], strerror(errno)); //If not executed, print in stderr
													exit(-1);
												}
												exit(-1); //This shouldn't be executed
											default:
													if (i > 0){ //If i > 0, we close the previous used pipe
														if (close(p[i-1][STDOUT_FILENO]) < 0){ //Close previous pipe in file table because it won't be using it and won't be replicated
															fprintf(stderr, "Error closing write for pipe %d: %s", i, strerror(errno));
														}
														if (close(p[i-1][STDIN_FILENO]) < 0){ //Close previous pipe in file table because it won't be using it and won't be replicated
															fprintf(stderr, "Error closing read for pipe %d: %s", i, strerror(errno));
														}
													}
													if (waitpid(cpid, &exit_code2, 0) < 0) { //Wait for the child to finish
														perror("Error when waitpid");
														return -1;
													}
													
										}
									}
									
								}
								exit(-1); //This should not be executed
							default: /* Parent process */
								if (!in_background){ //If not in background, we have to wait for the child to finish
									if (waitpid(pid, &exit_code, 0) < 0) {
										perror("Error when waitpid");
										return -1;
									}
								} else { //If in background, we have to finish the zombies
									struct sigaction sa;
									sa.sa_handler = &handle_sigchld;
									sigemptyset(&sa.sa_mask);
									sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
									if (sigaction(SIGCHLD, &sa, 0) == -1) {
										perror(0);
										exit(1);
									}
								}
						}
					}
                }
              }
        }
	return 0;
}
