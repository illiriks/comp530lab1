/* COMP 530: Tar Heel SHell */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024

bool is_empty(char s[]) {
	int k=0;
	while (s[k] != '\0') {
	  if (!isspace(*s)) return false;
		k++;
	}
	return true;
}

//Executes a command. Program is in argv[0], args are in argv.
void execute(char ** argv, bool debug){
	int pid;
	int status;
	// Fork process
	pid = fork();
	if (pid == 0) { // Child
		// Execute child
		if (execvp(*argv, argv) < 0){
			exit(1);
		}
	} else {
		if (debug){
			printf("RUNNING: %s\n", argv[0]);
		}
		// Parent process should 'sleep' while child isn't done
		while (wait(&status) != pid);
		// Child is done here
		if (debug){
			printf("ENDED: %s (ret=%d)\n", argv[0], status);
		}
		char errorcode[4];
		sprintf(errorcode, "%d", status);
		setenv("?", errorcode, 1);
	}
}

// Converts a string to an array of strings, tokenized
char ** parse(char * string) {
	// I don't know how to allocate 2d string arrays in C
	// but this works
	// although my laptop has 16GB RAM so it's OK
	char** returnme= malloc(1000000);
  int current = 0;
	const char s[2] = " ";
	char *token;
	token = strtok(string, s);
	// in case of empty string
	// while there is more string to tokenize,
	while (token != NULL ) {
		// add the token to the array and move forward
		if (token[0] == '$')
			token = getenv(token+1);
		returnme[current] = token;
		token = strtok(NULL, s);
		current ++;
	}
	return returnme;
}

int main (int argc, char ** argv, char **envp) {
  int finished = 0;
  char *prompt = "thsh> ";
  char cmd[MAX_INPUT];
	char cwd[1024];
	bool debug = false;
	// if the -d argument is passed, run this code in debug mode
	if ((argc > 1) && (strcmp(argv[1], "-d") == 0)){
			debug = true;
	}
	if ((argc > 1) && (access(argv[1], F_OK) != -1)){
		int pid = fork();
		if (pid == 0){
			// child
			int input = open(argv[1], O_RDONLY);
			dup2(input, 0);
			char ** args;
			execl("thsh", "thsh");
		} else {
			exit(3);
		}
	}
	// main loop
  while (!finished) {
    char *cursor;
    char last_char;
    int rv;
    int count;
    // Print the prompt
		write(1, "[", 2);
		// print cwd in prompt
    write(1, getcwd(cwd, sizeof(cwd)), strlen(getcwd(cwd, sizeof(cwd))));
		write(1, "] ", 3);
		rv = write(1, prompt, strlen(prompt));
    if (!rv) {
      finished = 1;
      break;
    }
    // read and parse the input
    for(rv = 1, count = 0, cursor = cmd, last_char = 1;
				rv && (++count < (MAX_INPUT-1)) && (last_char != '\n');
				cursor++) {
      rv = read(0, cursor, 1);
      last_char = *cursor;
    }
	 // TODO: Handle EOF like a newline. This will finish script support.
	 	*(cursor-1) = '\0';
		// if read fails
    if (!rv) {
      finished = 1;
      break;
    }
		// put builtin commands here
		if (!is_empty(cmd) && (cmd[0] != '#')) {
			char ** parsed_command = parse(cmd);
			if (strcmp(parsed_command[0], "exit") == 0) {
				exit(3);
			} else if (strcmp(parsed_command[0], "cd") == 0) {
				char * current_dir; char * prev_dir;
				// no argument to "cd" goes to home directory
				if (!parsed_command[1]) {
					prev_dir = getcwd(NULL, 0);
					chdir(getenv("HOME"));
					current_dir = getcwd(NULL, 0);
				} else if (strcmp(parsed_command[1], "~") == 0) {
					prev_dir = getcwd(NULL, 0);
					chdir(getenv("HOME"));
					current_dir = getcwd(NULL, 0);
				} else if (strcmp(parsed_command[1], "-") == 0) {
					current_dir = getcwd(NULL, 0);
					chdir(prev_dir);
					prev_dir = current_dir;
					current_dir = getcwd(NULL, 0);
				} else {
					prev_dir = getcwd(NULL, 0);
					chdir(parsed_command[1]);
					current_dir = getcwd(NULL, 0);
				}
			} else if (strcmp(parsed_command[0], "set") == 0){
				char * variable = strtok(parsed_command[1], "=");
				char * value = strtok(0, "=");
				setenv(variable, value, 1);
			}
			else execute(parsed_command, debug);
		}
  }
  return 0;
}
