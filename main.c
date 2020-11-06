/* 
 * main.c
 * Kia Pezesh V00845475
 * CSC360 Summer 2020, Assignment 1
 * A simple program to illustrate the use of the GNU Readline library 
 * Readline library is applied to a simple shell program
 * 
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT  15
#define MAX_SIZE 256
#define MAX_BG_JOBS 5

//struct to keep track of bg jobs
struct bg_job {
	char command[MAX_SIZE];
	pid_t pid;
	int status; //-1 = stopped, 0 = NULL, 1 = running

};

//initialize array of background job slots
struct bg_job bg_jobs[MAX_BG_JOBS];

//list information regarding bg job slots (and whatever bg jobs they may contain)
void bg_list(){
	int i;
	int count = 0; //initialize as 0 in case all slots are empty
	char s; //status char for printing
	printf("Printing background jobs: \n");
	//check every bg job slot
	for (i=0;i<MAX_BG_JOBS;i++){
		//convert int status to char s for printing
		if (bg_jobs[i].status > 0) s = 'R';
		else if (bg_jobs[i].status < 0) s = 'S';
		else {//s = '\0'; 
			printf("%d : Empty\n", i); 
			continue;
		}
		//print info and increment count
		printf("%d [%c]: command '%s', pid %x\n",i, s, bg_jobs[i].command, bg_jobs[i].pid); 
		if (bg_jobs[i].pid) count++;
	}
	printf("Total Background Jobs: %d\n", count);
}

//sends SIGSTOP to specified bg job 
void bg_stop(char* args[]){
	if(!args[1] || (args[1] && strlen(args[1])>1)){
		printf("Invalid input. Please specify which job to stop. eg. 'stop 1' to stop the process in slot 1.\n");
		return;
	}
	//convert char input to int
	int j = atoi(args[1]);
	if (j>4){
		printf("Invalid input. Background jobs comprise only of slots 0 to 4.\n");
	}else{
		if (bg_jobs[j].status < 0){
			printf("Background process in slot %d is already stopped.\n",j);
			return;
		}
		if (!bg_jobs[j].pid){
			printf("Slot %d is empty.\n", j);
			return;
		}
		kill(bg_jobs[j].pid, SIGSTOP);
		printf("Stop signal sent to background job slot %d\n", j);
		bg_jobs[j].status = -1;
	}
}

//sends SIGCONT to specified bg job
void bg_start(char* args[]){
	if(!args[1] || (args[1] && strlen(args[1])>1)){
		printf("Invalid input. Please specify which job to start. eg. 'start 1' to start the process in slot 1.\n");
		return;
	}
	int j = atoi(args[1]);
	if (j>4){
		printf("Invalid input. Background jobs comprise only of slots 0 to 4.\n");
	}else{
		if (bg_jobs[j].status > 0){
			printf("Background process in slot %d is already running.\n",j);
			return;
		}
		if (!bg_jobs[j].pid){
			printf("Slot %d is empty.\n", j);
			return;
		}
		kill(bg_jobs[j].pid, SIGCONT);
		printf("Start signal sent to background job slot %d\n", j);
		bg_jobs[j].status = 1;
	}
}
//sends SIGTERM to specified bg job
void bg_kill(char* args[]){
	if(!args[1] || (args[1] && strlen(args[1])>1)){
		printf("Invalid input. Please specify which job to kill. eg. 'bgkill 1' to kill the process in slot 1.\n");
		return;
	}
	int j = atoi(args[1]);
	if (j>4){
		printf("Invalid input. Background jobs comprise only of slots 0 to 4.\n");
	}else{
		if (!bg_jobs[j].status){
			printf("No process is currently running in background slot %d.\n",j);
			return;
		}
		if (!bg_jobs[j].pid){
			printf("Slot %d is empty.\n", j);
			return;
		}
		//if process in slot j is stopped, start it before sending kill signal.
		//Otherwise leaves a pseudo-zombie (the process is still exited upon closing the shell but is no longer kept track of in bglist)
		if (bg_jobs[j].status < 0) {
			kill(bg_jobs[j].pid, SIGCONT);
			bg_jobs[j].status = 1;
		}
		kill(bg_jobs[j].pid, SIGTERM); 
		bg_jobs[j].status = 0;
		printf("Kill signal sent to background job slot %d\n", j);
	}
}
//child process clean-up upon exiting parent process (ensure no zombies).
void bgkill(int n){
	kill(bg_jobs[n].pid, SIGTERM);
}
//execute specified command in the foreground
void fgexec (char* args[]){ 
	pid_t pid = fork();
	if (pid < 0) { //this should never be reached
		printf("fork() FAILED. \n");
	}
	if (pid == 0) { //child process
			/** 	for debugging
			* printf("Child process started. About to exec...");
			* int rc = execvp(args[0], args);
			*/
		execvp(args[0], args);
		//only reaches this point upon failed execution 
		printf("Invalid input. Specified command not found.\n");
			/**		for debugging
			*printf("execvp returned. ERROR in execvp execution: %d.\n", rc);
			*printf("Sending kill signal. \n");
			*/
		kill(getpid(),SIGTERM); //NOTE: waitpid() w/ WNOHANG on the SIGTERM terminated process returns status 'f'
		printf("ERROR: kill signal failed.\n");

	} else { //pid > 0 ~~ parent process
		int status;
		wait(&status); //stuck here until child terminates
			/** 	for debugging
			* id_t cpid = wait(&status); //stuck here until child terminates
			* printf("Child pid: %x\n", pid);
			* printf("Child finished: status: %x 		cpid:%x\n", status, cpid);
			*/
	}
} 
//execute specified command in the background
void bgexec (char* args[]){
	pid_t pid = fork();
	if (pid < 0) { //this should never be reached
		printf("BG: fork() FAILED. \n");
	}
	if (pid == 0) { //child process
			/** 	for debugging
			* printf("BG: Child process started. About to exec...\n");
			* int rc = execvp(args[0], args);
			*/
		execvp(args[0], args);
		//only reaches thi point upon failed execution
		printf("Invalid input. Specified command not found.\n");
			/** 	for debugging
			* printf("BG: execvp returned. ERROR in execvp execution: execvp() returns %d.\n", rc);
			* printf("Sending kill signal. \n");
			*/
		kill(getpid(),SIGTERM); 
		printf("ERROR: kill signal failed.\n");
	} else { //pid > 0 ~~ parent process
		int i;
		//printf("child id: %x\n", pid);
		for (i=0;i<MAX_BG_JOBS; i++){
			if (!bg_jobs[i].pid){
				bg_jobs[i].pid = pid;
				bg_jobs[i].status = 1;
				int j=1;
				strcpy(bg_jobs[i].command, args[0]); //copy first args token into bg_jobs[].command
				while(args[j]){ //copy remaining tokens into bg_jobs[].command
					strcat(bg_jobs[i].command, " ");
					strcat(bg_jobs[i].command, args[j]);
					j++;
				}
				break; //exit for loop once open slot was found
			}
		}
	}
}
/**checks user input args for built in commands.
 * return 1 if shell is to continue to next iteration of its for loop.
 * return 0 if user did not invoke a built in command. 
 */
int built_in_check(char* args[]){
		//built in 'cd' command
		if (!strcmp(args[0],"cd")) {
			if (args[2]) {
				printf("Incorrect input arguments for command: cd \n");
				return 1;
			}
			if (chdir(args[1]) != 0) printf("cd to '%s' failed.\n", args[1]);
			return 1;
		}
		//list bg jobs
		if (!strcmp(args[0],"bglist")){
			bg_list();
			return 1;
		}
		//kill specified bg job
		if(!strcmp(args[0], "bgkill")){
			bg_kill(args);
			return 1;
		}
		//pause specified bg job
		if(!strcmp(args[0], "stop")){
			bg_stop(args);
			return 1;
		}
		//continue specified bg job
		if(!strcmp(args[0], "start")){
			bg_start(args);
			return 1;
		}
		//user input error handling:
		if(!strcmp(args[0], "bgstop")){
			printf("Unknown command. Try 'stop' instead.\n");
			return 1;
		}
		if(!strcmp(args[0], "bgstart")){
			printf("Unknown command. Try 'start' instead.\n");
			return 1;
		}
		if(!strcmp(args[0], "kill")){
			printf("Opertation not permitted. Try 'bgkill' instead.\n");
			return 1;
		}
		if(!strcmp(args[0], "list")){
			printf("Unknown command. Try 'bglist' instead.\n");
			return 1;
		}
	return 0;
}

/** parse method takes tokenized user input from cmd
 *	and copies only the first 15 items into a new char* variable
 * 	before returning. 
*/	
void parse (char* input, char* args[]) {
	int i=0;
	while(input){
		args[i]=input;
		input = strtok(NULL, " "); //move input to next token
		//printf("%s, ",args[i]); //DEBUG
		i++;
		if (i == 15 ) break; //ASSUMPTION: MAX 14 PARAMETERS IN A COMMAND
		}
	args[i] = (char *)0; //end with NULL
}

int main (int argc, char * argv[]){
	for (;;) {
		sleep(0.2); //avoids some instances of child processes printing after ">"
		char cwd[MAX_SIZE]; //'cwd'=='current working directory'
		getcwd(cwd, (size_t)MAX_SIZE);
		printf ("%s", cwd);
		char *cmd = readline (">");
		//printf ("Got: [%s]\n", cmd);

		//auto-clearing terminated background job slots
		int i, rc, status;
		for (i=0;i<MAX_BG_JOBS; i++){ 
			rc = waitpid(bg_jobs[i].pid, &status, WNOHANG);
			if (bg_jobs[i].pid && (!bg_jobs[i].status || rc == -1)){
				printf("Background process in slot %d has terminated.\n", i); //assignment pdf specifies user must be updated upon bg process terminating
				bg_jobs[i].pid = 0;
				bg_jobs[i].status = 0;
				strcpy(bg_jobs[i].command, "");
			}
		}

		//tokenize input
		char *input = strtok(cmd, " ");
		char *args[MAX_INPUT];
		//check if input is empty; if so, continue loop
		if (!input) continue;
		//assume fg execution unless...
		int bg = 0;
		if (!strcmp(input, "bg")) {
			bg = 1; 
			input = strtok(NULL, " "); //increment to next token (preparation for parse() to limit input to 15 args)
			if (!input) {
				printf("Error: missing arguments for command 'bg'\n");
				free (cmd);
				continue;
			}
		}
		//parse tokenized input and uphold MAX_INPUT constraint. Copy result into args
		parse(input, args);

		//if bg==1, execute specified command in the background ()perform necessary logic beforehand to check if slots are available)
		if (bg){
			//printf("BACKGROUND EXECUTION\n");
			int count=0;
			for(int i=0;i<MAX_BG_JOBS;i++){
				if(bg_jobs[i].pid) count++;
			}
			if (count>4) {
				printf("Background job slots are full. Use 'kill n' command to free slot n.\n");
				free(cmd);
				continue;
			}
			bgexec(args);
			free(cmd);
			continue;
		}

		//built in 'exit' command
		if (!strcmp(args[0],"exit")){
			for(int i=0;i<MAX_BG_JOBS;i++){ //kill every child process before exiting the shell
				if (bg_jobs[i].pid) bgkill(i);
			}
			free(cmd);	
			break;//exit for loop, return 0;
		}
		//built in 'pwd' command. (Unnecessary b/c execvp() can do this but included as per a1 part II desc.)
		if (!strcmp(args[0],"pwd")) { //didn't put this into built_in_check() so I wouldn't have to re-obtain cwd
			printf("%s\n", cwd);
			free(cmd);
			continue;
		}
		//check for rest of built in commands (just to clean up main() a little)
		if (built_in_check(args)){
			free(cmd);
			continue;
		}

		//if user input was not a built in command nor set for background execution, execute in the foreground.
		//printf("FG execution starting...\n");
		fgexec(args);
		free (cmd);
	}	
	return 0;
}

/**	BUG: if user inputs an invalid command to run in the background, eg. 'bg asda', the command will still show as a process upon calling bglist for the next iteration of the shell.
 *	tried using waitpid() with WNOHANG to detect a failed execution in its child process and stop the command from being placed into bglist, but upon immediate execution of waitpid() in the parent,
 *	it is returning 0 (which is the same as a regular, successfully executed process). 
 *	I think this could be fixed if I were to allocate some shared memory to have a shared variable between child/parent to flag whether this is the case or not, but I believe that is beyond the scope of this assignment.  
 */
