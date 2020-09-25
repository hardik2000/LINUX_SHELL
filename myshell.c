/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the signal handler code (which should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you dp not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#include <limits.h>			//PATH_MAX

#define BUFSIZE 64
#define DELIM " \t\r\n\a"
enum types{SEQUENTIAL,PARALLEL,OUTPUT,SIMPLE};	//to improve readability of code ,for understanding which kind of operation

void changedir(char ** outputs)
{
    if(chdir(outputs[1])!=0){
	    printf("Shell: Incorrect command\n");
    }
}   // when cd is called

char **parseInput(char *line)
{
	// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
	int bufsize = BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) 
	{
		//perror("allocation error");
		//exit(EXIT_FAILURE);
	}

	token = strtok(line, DELIM);
	while (token != NULL) {
	    tokens[position] = token;
	    position++;

	    if (position >= bufsize) {
	    	bufsize += BUFSIZE;
	    	tokens = realloc(tokens, bufsize * sizeof(char*));
	    	if (!tokens) {
	   			//perror("allocation error");
				// exit(EXIT_FAILURE);
	      	}
	    }

	    token = strtok(NULL, DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

void executeCommand(char **outputs)
{
	// This function will fork a new process to execute a command
	if(strcmp(outputs[0],"cd")==0){
        changedir(outputs);
    }
    else
    {
		pid_t pid,wpid;
		int status;

		pid = fork();
		if (pid < 0){			// fork failed; exit
			exit(0);
		}
		else if (pid == 0) {		// child (new) process
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			if(execvp(outputs[0], outputs)==-1)
			{
				printf("Shell: Incorrect command\n");
	            exit(1);
			}
			exit(0);
			
		} 
		else {              // parent process (rc holds child PID)
			waitpid(pid, &status, WUNTRACED );
		}
	
	}
}

void executeParallelCommands(char **outputs)
{
	// This function will run multiple commands in parallel
	pid_t pidt;
	int status=0;
		int i=0;
		int prev_index=0; 
		while(outputs[i])
		{
			while(outputs[i] && strcmp(outputs[i],"$$")!=0)
			{
				i++;
			}
			outputs[i]=NULL;
			
			if(!strcmp(outputs[prev_index],"cd"))
			{
				changedir(outputs);
		    }
		    else
		    {
				//forking a new process for execution of cmd		
				pidt = fork();
			
				if (pidt < 0){			// fork failed; exit
					exit(0);
				}
				else if (pidt == 0) {		// child (new) process
					signal(SIGINT, SIG_DFL);			// Restore the default behavior for SIGINT signal
					signal(SIGTSTP, SIG_DFL);			// Restore the default behavior for SIGTSTP signal
					if(execvp(outputs[prev_index],&outputs[prev_index])==-1)
					{
						printf("Shell: Incorrect command\n");
			            exit(1);
					}
				}
			} 
			
			i++;
			prev_index=i;
		}
	// while((pidt=wait(&status))>0); 
	waitpid(pidt, &status, WUNTRACED );
	
	
}

void executeSequentialCommands(char **outputs)
{	
	// This function will run multiple commands in parallel
	int i=0;
	int prev_index=0; 
	while(outputs[i])
	{
		while(outputs[i] && strcmp(outputs[i],"##")!=0)
		{
			i++;
		}
		outputs[i]=NULL;
		executeCommand(&outputs[prev_index]);
		i++;
		prev_index=i;
	}

}

void executeCommandRedirection(char **outputs)
{
	int count=0,i=0;
	int status;
	while(outputs[i])
	{
		count++;
		i++;
	}
	// This function will run a single command with output redirected to an output file specificed by user
	int rc = fork();
	
	if (rc < 0){			// fork failed; exit
		exit(0);
	}
	else if (rc == 0) {		// child (new) process
	
		signal(SIGINT, SIG_DFL);			// Restore the default behavior for SIGINT signal
		signal(SIGTSTP, SIG_DFL);			// Restore the default behavior for SIGTSTP signal
		// ------- Redirecting STDOUT --------
		
		close(STDOUT_FILENO);
		int f=open(outputs[count-1], O_CREAT | O_WRONLY | O_APPEND, 0777);
		if(f==-1){
		//	        perror("opening cout.log");
	    }
	    int save_out = dup(fileno(stdout));
	    if(dup2(f,fileno(stdout)==-1)){
		//	        perror("cannot redirect stdout");
	    }

        // -----------------------------------
		char **execCommand;
		execCommand=(char**)malloc(sizeof(char*)*(count-1));
		int i=0;
		for(;i<(count-2);i++)
		{
			execCommand[i]=outputs[i];
		}
		execCommand[i]=NULL;
		if(execvp(execCommand[0], execCommand)==-1)
		{
			printf("Shell: Incorrect command\n");
            exit(1);
		}

        //closing file descriptor
	    fflush(stdout); close(f);
	    dup2(save_out, fileno(stdout));
	    close(save_out);
		
	} 
	else {              // parent process (rc holds child PID)
		// int rc_wait = wait(NULL);
		waitpid(rc, &status, WUNTRACED );
	
	}
}
char cwd[PATH_MAX];

void handle_sigint(int sig) 
{ 
	printf("\n");
	printf("%s$", cwd);
	fflush(stdout);

} 
  
int main()
{
	
	// Initial declarations
	signal(SIGINT, handle_sigint); 
	signal(SIGTSTP, handle_sigint); 
	 	
	while(1)	// This loop will keep your shell running until user exits.
	{
		// Print the prompt in format - currentWorkingDirectory$
		if (getcwd(cwd, sizeof(cwd)) != NULL) 
	   	{
	       printf("%s$", cwd);
	   	} else {
	       //perror("getcwd() error");
	       //break;
	   	}

		// accept input with 'getline()'
		char *line = NULL;
		ssize_t bufsize = 0; // have getline allocate a buffer for us
		if (getline(&line, &bufsize, stdin) == -1)
		{
			if (feof(stdin)) {
		        //exit(EXIT_SUCCESS);  // We recieved an EOF
		    } else {
	  	     	//   perror("readline");
		        // exit(EXIT_FAILURE);
			}
		}
		if(!strcmp(line,"\n"))
		{
			continue;
		}
			
		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		char **outputs=parseInput(line); 
		
		if(!strcmp(outputs[0],"exit") || !strcmp(outputs[0],"EXIT"))	// When user uses exit command.
		{
			printf("Exiting shell...\n");
			break;
		}
		//if user not exited then
		enum types ch=SIMPLE;		//type of variable which defines what kind of operation either parellel or sequential
		for(int i=0;outputs[i];i++)
		{
			if(!strcmp(outputs[i],"##"))
			{
				ch=SEQUENTIAL;
				break;
			}
			else if(!strcmp(outputs[i],"$$"))
			{
				ch=PARALLEL;
				break;
			}
			else if(!strcmp(outputs[i],">"))
			{
				ch=OUTPUT;
				break;
			}

			//printf("%s\n",outputs[i]);
		}
		if(ch==PARALLEL)
		{
			executeParallelCommands(outputs);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		}else if(ch==SEQUENTIAL)
		{
			executeSequentialCommands(outputs);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		}else if(ch==OUTPUT)
		{
			executeCommandRedirection(outputs);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		}else
		{
			executeCommand(outputs);		// This function is invoked when user wants to run a single commands
		}		
		
	}
	
	return 0;
}
