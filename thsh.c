/* COMP 530: Tar Heel SHell

* Name: Welsey Hitson
* UNC Honor Pledge: I certify that no unauthorized assistance has been received or given
* in the completion of this work.
* Signature: Wesley Hitson
* Collaborators: Illirik Smirnov

* Name:
* UNC Honor Pledge: I certify that no unauthorized assistance has been received or given
* in the completion of this work.
* Signature:
* Collaborators: Wesley Hitson

* References: Linux man pages (http://man7.org/linux/man-pages/index.html), Donald Porter's slides,

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
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

int countPipes(char * string){
	int pipes = 0;
	const char *tmp = string;
	while ((tmp = strstr(tmp, "|"))){
		pipes++;
		tmp++;
	}
	return pipes;
}

//Executes a command. Program is in argv[0], args are in argv.
void execute(char * cmd, bool debug, char * current_dir, char * prev_dir, int in_handle, int out_handle){
	char** argv = malloc(80000);
  int current = 0;
	const char s[2] = " ";
	char *token;
	token = strtok(cmd, s);
	// in case of empty string
	// while there is more string to tokenize,
	while (token != NULL) {
		// add the token to the array and move forward
		if (token[0] == '$')
			token = getenv(token+1);
		argv[current] = token;
		token = strtok(NULL, s);
		current ++;
	}
	if (strcmp(argv[0], "exit") == 0) {
		exit(3);
	} else if (strcmp(argv[0], "cd") == 0) {
		// no argument to "cd" goes to home directory
		if (!argv[1]) {
			strcpy(prev_dir, getcwd(NULL, 0));
			chdir(getenv("HOME"));
			strcpy(current_dir, getcwd(NULL, 0));
		} else if (strcmp(argv[1], "~") == 0) {
			strcpy(prev_dir, getcwd(NULL, 0));
			chdir(getenv("HOME"));
			strcpy(current_dir, getcwd(NULL, 0));
		} else if (strcmp(argv[1], "-") == 0) {
			strcpy(current_dir, getcwd(NULL, 0));
			chdir(prev_dir);
			strcpy(prev_dir, current_dir);
			strcpy(current_dir, getcwd(NULL, 0));
		} else {
			strcpy(prev_dir, getcwd(NULL, 0));
			chdir(argv[1]);
			strcpy(current_dir, getcwd(NULL, 0));
			if (strcmp(prev_dir, current_dir) == 0) {
				printf("Could not find directory \'%s\'\n", argv[1]);
			}
		}
	} else if (strcmp(argv[0], "set") == 0){
		char * variable = strtok(argv[1], "=");
		char * value = strtok(0, "=");
		setenv(variable, value, 1);
	} else if (strcmp(argv[0], "goheels") == 0) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
		FILE *file;
    file = fopen("goheels.txt", "r");
    if (file == NULL) {
        printf("File does not exist.\n");
				exit(3);
		}
    while ((read = getline(&line, &len, file)) != -1) {
        printf("%s", line);
    }
		free(line);
    fclose(file);
	} else {
		int pid;
		int status;
		// Fork process
		pid = fork();
		if (pid == 0) { // Child
			// Execute child
			if (in_handle != 0){
				dup2(in_handle, 0);
			}
			if (out_handle != 0){
				dup2(out_handle, 1);
			}
			if (execvp(*argv, argv) < 0){
			  printf("Unrecognized command: \'%s\'\n", argv[0]);
				exit(1);
			}
		} else {
			if (debug){
				printf("RUNNING: %s\n", argv[0]);
			}
			if (in_handle != 0){
				close(in_handle);
			}
			if (out_handle != 0){
				close(out_handle);
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
}

void parse(char * cmd, bool debug, char * current_dir, char * prev_dir) {
	// int current = 0; 	// unused
	int k;
	int numPipes = countPipes(cmd);
	char pipeString[2] = "|";
	int pipes[numPipes][2];
	for (k=0; k<numPipes; k++) {
		pipe(pipes[k]);
	}
	char * commands[numPipes + 1];
	commands[0] = strtok(cmd, pipeString);
	for (k=1; k<=numPipes; k++) {
		commands[k] = strtok(NULL, pipeString);
	}
	for (k=0; k<=numPipes; k++) {
		if ((k == 0) & (k == numPipes)){
			execute(commands[k], debug, current_dir, prev_dir, 0, 0);
		} else if (k == 0) {
			execute(commands[k], debug, current_dir, prev_dir, 0, pipes[0][1]);
		} else if (k == numPipes){
			execute(commands[k], debug, current_dir, prev_dir, pipes[k-1][0], 0);
		} else {
			execute(commands[k], debug, current_dir, prev_dir, pipes[k-1][0], pipes[k][1]);
		}
	}
}

int main (int argc, char ** argv, char **envp) {
	char * current_dir = getcwd(NULL, 0);
	char * prev_dir = getcwd(NULL, 0);
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
			// char ** args;	// this line was throwing a warning during compile
			execl("thsh", "thsh", NULL);
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
		// then execute the command
		if (!is_empty(cmd) && (cmd[0] != '#')) {
			parse(cmd, debug, current_dir, prev_dir);
		}
  }
  return 0;
}
