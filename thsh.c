/* COMP 530: Tar Heel SHell */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024

void execute(char ** argv){
	int pid;
	int status;

	pid = fork();
	if (pid == 0) { // Child
		if (execvp(*argv, argv) < 0){
			printf("%s", *argv);
			exit(1);
		}
	} else {
		while (wait(&status) != pid);
	}
}

char ** parse(char * string) {
	char ** returnme = malloc(sizeof(char*) * MAX_INPUT / 2);
  int current = 0;
	const char s[2] = " ";
	char *token;
	token = strtok(string, s);
	while (token != NULL ) {
		*(returnme + current) = token;
		token = strtok(NULL, s);
		current ++;
	}
	return returnme;
}

int 
main (int argc, char ** argv, char **envp) {
  int finished = 0;
  char *prompt = "thsh> ";
  char cmd[MAX_INPUT];


  while (!finished) {
    char *cursor;
    char last_char;
    int rv;
    int count;


    // Print the prompt
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    }
    
    // read and parse the input
    for(rv = 1, count = 0, 
	  cursor = cmd, last_char = 1;
	rv 
	  && (++count < (MAX_INPUT-1))
	  && (last_char != '\n');
	cursor++) { 

      rv = read(0, cursor, 1);
      last_char = *cursor;
    } 
	 	*(cursor-1) = '\0';

    if (!rv) { 
      finished = 1;
      break;
    }

		char ** parsed_command = parse(cmd);
		if (strcmp(*parsed_command, "exit") == 0){
			exit(3);
		}
    // Execute the command, handling built-in commands separately 
    // Just echo the command line for now
		execute(parsed_command);
    //write(1, cmd, strnlen(cmd, MAX_INPUT));

  }

  return 0;
}
