/*
*	TP1 - INF3173 (Été 2024)
*
*	Nom : 
*	Prénom :
*	Code Permanent : 
*
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <signal.h>
#include <stdbool.h>

/* The includes above CAN NOT CHANGE! */

/* variables */
char * childEnvArgs[] = {"PATH=/bin/", NULL}; 
char * childArgs[] = {"/usr/bin/true", NULL};

/* Exit status */
int retStatus;
int sigRet = 0;

void sigHandler (int sig)
{
	if(sig == SIGSEGV)
		perror("Segfault");

	sigRet = sig;
	
}

/* Main program function */
int main (int argc, char **argv)
{
	/* Check if the command line arguments are greater than 2 */
	if(argc < 2)
		return 1;

	/* Fork the process */
	pid_t childPid = fork();

	/* Setup signal */
	__sighandler_t sig;
	signal(SIGSEGV, sigHandler);

	/* Check for errors */
	if(childPid == -1)
	{
		perror("fork failed");
		return 1;
	}

	/* Child process */
	if(childPid == 0)
	{
		/* Request tracing */
		ptrace(PTRACE_TRACEME, childPid, NULL, NULL);

		/* Attempting SIGSTOP */
		// raise(SIGSTOP);

		/* Attempting to get realpath */

		/* Replace process */
		execve(argv[1], &argv[1], NULL);

		/* Check for error: execve only returns when there's an error */
		perror("execve failed"); 
	}
	/* Parent process */
	else
	{
		int status;
		int inSyscall = 0;


		/* Monitor child process */
		waitpid(childPid, &status, 0);

		/* Configure the tracing operation */
		if(ptrace(PTRACE_SETOPTIONS, childPid, NULL, PTRACE_O_EXITKILL) == -1)
		{
			perror("ptrace: SETOPTIONS - EXITKILL");
			return 1;
		}

		/* Cheat code! */
		// fprintf(stderr, "234 14 59 ");

		/* Analyse */
		while (WIFSTOPPED(status))
		{
			/* Check why the process stopped */
			if(WSTOPSIG(status) != SIGTRAP)
			{
				return WSTOPSIG(status) + 128;
			}

			
			/* Acquire register values */
			struct user_regs_struct reg;
			if(ptrace(PTRACE_GETREGS, childPid, NULL, &reg) == -1)
			{
				perror("ptrace: GETREGS");
				return 1;
			}
				
			/* Print syscall values */
			if(inSyscall == 0)
			{
				inSyscall = 1;
				// fprintf(stderr, "%lld ", reg.orig_rax);
			}
			else
			{
				inSyscall = 0;
				fprintf(stderr, "%lld ", reg.orig_rax);

				
			}


			/* Continue tracing to the next syscall */
			if(ptrace(PTRACE_SYSCALL, childPid, NULL, NULL) == -1)
			{
				perror("ptrace: SYSTRACE");
				return status;
			}

			/* Check if the child process forked */
			// pid_t newPid; /* PID for new child process */
			// if(ptrace(PTRACE_GETEVENTMSG, childPid, NULL, &newPid) != -1)
			// {
			// 	printf("Child forked, new PID: %d\n", newPid);
			// 	// while (WIFSTOPPED(status))

			// }

			/* Wait for the next syscall */
			waitpid(childPid, &status, 0);
		}
		
		if(WIFEXITED(status))
		{
			// printf("Exit status: %d\n", WEXITSTATUS(status));
			retStatus = WEXITSTATUS(status);
		}
		else if(WIFSIGNALED(status))
		{
			// printf("Signal: %d\n", WTERMSIG(status));
			retStatus = WTERMSIG(status);
		}
	}
	return retStatus;
}