// TODO TODO TODO TODO TODO TODO TODO 
/*
 - break string into an array of strings at start
  - replace all strcmps to cmd[i]
- fix background 
  - make an vector of processes? 
- ^Z on fg process suspends it
  - Cause the foreground process to be suspended.
- jobs, kill, bg, fg

*/
// TODO TODO TODO TODO TODO TODO TODO 


// Name:		Ben Belden
// Class ID#:	bpb2v
// Section:		CSCI 6250-001
// Assignment:	Lab #2
// Due:			16:20:00, October 12, 2016
// Purpose:		Write a C/C++ program that implements a portion of my own shell.		
// Input:	    From terminal.	
// Outut:		To terminal.
// 
// File:		p3.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
#include <vector>
#include <string>
#define OK       0
#define NO_INPUT 1
#define EOFILE   2
#define HASHSIZE 101
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
// typedef enum {false,true} bool;

// all hash functions from The C Programming Language p. 144-5
struct nlist {
	struct nlist *next;
	std::string *name;
	std::string *val;
};

struct susp {
	bool isSusp;
	int suspPid;
};
struct susp s1;

std::vector<int> jobsVector;
unsigned hash (std::string s)
{
	unsigned hashval = 0;
	for (int i = 0; s[i] != '\0'; i++)
		hashval = s[i] + 31 + hashval;
	return hashval % HASHSIZE;
}

struct nlist *lookup(std::string s, struct nlist *hashtab[HASHSIZE])
{
	struct nlist *np;
	for (np = hashtab[hash(s)]; np != NULL; np = np->next)
		if (s==*np->name)
			return np; // found
	return NULL; // not found
}

struct nlist *insert(std::string name, std::string val, struct nlist *hashtab[HASHSIZE])
{
	struct nlist *np;
	unsigned hashval;

	if ((np = lookup(name,hashtab)) == NULL) // not found 
	{
		np = (struct nlist *) malloc(sizeof(*np));
		if (np == NULL || np->name == NULL)
			return NULL;
		hashval = hash(name);
		np->next = hashtab[hashval];
		hashtab[hashval] = np;
	}
	else // found
		np->val=NULL; // free previous val
	if (np->val == NULL)
		return NULL;
	return np;
}

static int getLine (char *prmpt, char *buff, size_t sz)
{
    char *charPos;

	if (prmpt != NULL) 
	{
		if (isatty(fileno(stdin))) // only if tty
			printf (ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, prmpt);
		fflush (stdout);
	}

	// get line and check for EOF
	if (fgets (buff, sz, stdin) == NULL)
	{
		printf("\n");
		return EOFILE;
	}

	// only new line
	if (buff[0] == '\n') return NO_INPUT;

	// remove newline 
	buff[strlen(buff)-1] = '\0';

	// trim inline comments
	charPos = strchr(buff,'#');
	if (charPos != NULL)
		buff[((int)(charPos-buff))] = '\0';

	return OK;
}

int isValidName (char *var)
{
	const char first[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
	const char rest[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_1234567890";
	int len;
	char *firstChar = &var[0];
	// check to see if var begins with alpha or underscore
	len = strspn(firstChar,first);
	if (len == 0)
		return false;
	// check to see if var contains only alphas, numerics, and underscores
	len = strspn(var,rest);
	if (len != strlen(var))
		return false;
	// otherwise
	return true;
}

static struct nlist *shellVarHash[HASHSIZE];
static struct nlist *envVarHash[HASHSIZE];

// interpolate var
char * interp(char * s)
{
	s++;
	char * retStr;
	retStr = (char *)lookup(s,shellVarHash)->val;
	if (retStr == NULL)
		retStr = (char *)lookup(s,envVarHash)->val;
	printf("%s\n",retStr);
	return retStr; 
}

pid_t pid = 0,childPid;
int status;
void sigchld_handler(int s)
{
	while(waitpid(-1,&status,WNOHANG)>0);
	printf("\n[1]+  Done\t%d\n",pid);
	jobsVector.erase(std::remove(jobsVector.begin(), jobsVector.end(), pid), jobsVector.end());
}

void sigstop(int s)
{
	//signal(SIGTSTP,SIG_DFL);
	printf("pid: %d stopped\n",pid);
	//while(waitpid(-1,&status,WNOHANG)>0);
	//while(waitpid(-1,&status,WUNTRACED)>0);
	//waitpid(pid,NULL,WNOHANG);
	kill(pid,SIGSTOP);
	jobsVector.push_back(pid);
	s1.isSusp = true;
	s1.suspPid = pid;
	printf("\nyeah, this still isn't working right.\n");
	printf("if you want to see applicable code, look at lines:\n");
	printf("166-208 and 572-593\n\n");
	printf("you can still start a background process with &\n");
	printf("and run fg, jobs, and kill\n");
	fflush(stdout);
}

void sigcontfg(int s)
{
	kill(s,SIGCONT);
	jobsVector.erase(std::remove(jobsVector.begin(), jobsVector.end(), pid), jobsVector.end());
}

void sigcontbg(int s)
{
	kill(s,SIGCONT);
}

void sigkill(int s)
{
	kill(s,SIGTERM);
	jobsVector.erase(std::remove(jobsVector.begin(), jobsVector.end(), pid), jobsVector.end());
}

int main (int argc, char *argv[])
{
	setbuf(stdout,NULL);
	clearenv();
	int i,rc,done=false,loopDone=false;
	//long timeLim=0;
	int timeLim=0;
	// char buff[100],*tok,*pathTok,*rest,*name,*val,syntax[]="syntax",nullTerm[]="\0";
	// char cwd[1024],*buff_cpy;
	char *tok,*pathTok,cwd[1024],buff[100],*buff_cpy;
	std::string rest,name,val,syntax="syntax",nullTerm="\0";
	const char command[]="which ",slash[]="/",exitStr[]="exit",set[] = "set";
	const char unset[]="unset",prt[]="prt",envset[]="envset",envunset[]="envunset";
	const char envprt[]="envprt",witch[]="witch",pwd[]="pwd",cd[]="cd",lim[]="lim";
	const char jobs[]="jobs",kill[]="kill",fg[]="fg",bg[]="bg";
	const char *cmds[15];
	cmds[0]="exit"; cmds[1]="set"; cmds[2]="unset"; cmds[3]="prt"; cmds[4]="envset";
	cmds[5]="envunset"; cmds[6]="envprt"; cmds[7]="witch"; cmds[8]="pwd"; cmds[9]="cd";
	cmds[10]="lim"; cmds[11]="jobs"; cmds[12]="kill"; cmds[13]="fg"; cmds[14]="bg";
	struct nlist *np;
	insert("AOSPATH","/bin:/usr/bin",envVarHash);
	if (getcwd(cwd, sizeof(cwd)) != NULL)
		insert("AOSCWD",cwd,envVarHash);
	setenv("AOSCWD",cwd,0);
	setenv("AOSPATH","/bin:/usr/bin",0);
	struct rlimit rl;
	getrlimit(RLIMIT_CPU,&rl);
	timeLim = rl.rlim_max;
	s1.isSusp = false;
	s1.suspPid = 0;

	while (!done)
	{
		char prmpt[] = "bpb2v_sh> ";
		rc = getLine (prmpt, buff, sizeof(buff));

		while (rc == NO_INPUT)
			rc = getLine (prmpt, buff, sizeof(buff));
		buff_cpy = (char *)malloc(sizeof(buff));
		strcpy(buff_cpy,buff);

		// break input line into array of strings
		char **cmdArray = NULL;
		char *p = strtok(buff," ");
		int n_spaces = 0;
		while (p)
		{
			cmdArray = (char **)realloc(cmdArray,sizeof (char *)*(++n_spaces));
			if (cmdArray == NULL) break;
			cmdArray[n_spaces-1]=p;
			p = strtok(NULL," ");
		}
		cmdArray = (char **)realloc(cmdArray,sizeof(char *)*(++n_spaces));
		cmdArray[n_spaces-1]='\0';
		
		// if cmd is exit or EOF or CTRL-D
		if (strcmp(cmdArray[0],exitStr) == 0 || rc == EOFILE)
			break;
		 
		// if cmd is pwd
		else if (strcmp(cmdArray[0],pwd) == 0)
		{
			np = lookup("AOSCWD",envVarHash);
			// TODO why why why why why why???
		//	printf("%s\n",np->val); 
		}

		// if cmd is envprt
		else if (strcmp(cmdArray[0],envprt) == 0)
		{
			system("printenv");
			/*
			for(i=0;i<HASHSIZE;i++)
				if (envVarHash[i] != NULL)
					if (envVarHash[i]->val != NULL && (*envVarHash[i]->val)[0] != '\0')
						printf("%s=%s\n",(*envVarHash[i]->name).c_str(),(*envVarHash[i]->val).c_str());
						*/
		}

		// if cmd is jobs
		// Builtin command to list current jobs (suspended or background).
		// List each with a num beside it to be used in following commands.
		else if(strcmp(cmdArray[0],jobs)==0)
		{
			if(jobsVector.empty())
			{
				printf("no jobs\n");
				continue;
			}
			for(std::vector<int>::iterator it = jobsVector.begin();it != jobsVector.end(); it++)
				printf("%d\n",*it);
			continue;
		} // end if (strcmp(buff,jobs)==0)

		//  if cmd is set
   		// set var val  (shell variable - not env)  - ‘ “ not required
		else if (strcmp(cmdArray[0],set) == 0)
		{
			syntax = "syntax: set var val";
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntax: set var val";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			// check to see if name is valid
			if (!isValidName(cmdArray[1])) { printf("Invalid variable name.\n"); continue; }
			if (cmdArray[1][0] == '$') cmdArray[1] = interp(cmdArray[1]);
			if (strcmp(cmdArray[2],"\0") == 0) 
			{ 
				syntax = "syntax: set var val";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			if (cmdArray[2][0] == '$') cmdArray[2] = interp(cmdArray[2]);
			insert(cmdArray[1],cmdArray[2],shellVarHash);
		} // end if (strcmp(cmdArray[0],set) == 0)

		// if cmd is unset
   		// unset var    (shell variable - not env)
		else if (strcmp(cmdArray[0],unset) == 0)
		{
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntax: unset var";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			// check to see if name is valid
			if (!isValidName(cmdArray[1])) { printf("Invalid variable name.\n"); continue; }
			if (cmdArray[1][0] == '$') cmdArray[1] = interp(cmdArray[1]);
			insert(cmdArray[1],nullTerm,shellVarHash);
		} // end else if (strcmp(cmdArray[0],unset) == 0)

		// if cmd is prt
		// prt word $var (can have many args) 
		// (use shell's $var if same name in both shell and env)
		else if (strcmp(cmdArray[0],prt) == 0)
		{
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntax: prt var1 var2 var3 ... varN";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			i = 1;
			while (cmdArray[i] != 0)
			{
				if (cmdArray[i][0] == '$') 
				{	
					cmdArray[i] = cmdArray[i];
					char * temp = cmdArray[i];
					temp++;
					cmdArray[i] = temp;
					np = lookup(cmdArray[i],shellVarHash);
					if (np == NULL || np->val == '\0') np = lookup(cmdArray[i],envVarHash);
					if (np != NULL && np->val != '\0') printf("%s ",np->val);
				} // end if
				else printf("%s ",cmdArray[i]);
				i++;
			} // end while
			printf("\n");
		} // end else if (strcmp(tok,prt) == 0)

		// if cmd is envset
   		// envset VAR VAL
		else if (strcmp(cmdArray[0],envset) == 0)
		{
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntax: envset VAR VAL";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			if (cmdArray[1][0]=='$') cmdArray[1] = interp(cmdArray[1]);
			// check to see if name is valid
			if (!isValidName(cmdArray[1])) { printf("Invalid variable name.\n"); continue; }
			if (strcmp(cmdArray[2],"\0") == 0) 
			{ 
				syntax = "syntax: envset VAR VAL";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			if (cmdArray[2][0]=='$') cmdArray[2]=interp(cmdArray[2]);
			insert(cmdArray[1],cmdArray[2],envVarHash);
			setenv(cmdArray[1],cmdArray[2],0);
		} // end else if (strcmp(tok,envset) == 0)

		// if cmd is envunset
   		// envunset VAR
		else if (strcmp(cmdArray[0],envunset) == 0)
		{
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntax: envunset VAR";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			// check to see if name is valid
			if (!isValidName(cmdArray[1])) { printf("Invalid variable name.\n"); continue; }
			if (cmdArray[1][0]=='$') cmdArray[1]=interp(cmdArray[1]);
			insert(cmdArray[1],nullTerm,envVarHash);
			unsetenv(cmdArray[1]);
		} // end else if (strcmp(tok,envunset) == 0)

		// if cmd is witch
   		// witch (like which :)  (looks thru AOSPATH for a cmd - see below)
		else if (strcmp(cmdArray[0],witch) == 0)
		{
			printf("witch\n");
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntax: witch cmd";
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			// check to see if built in
			loopDone=false;
			for (i=0;i<15 && !loopDone;i++)
			{
				if(strcmp(cmdArray[1],cmds[i]) == 0)
				{
					printf("%s: shell built-in command.\n",tok);
					loopDone=true;
				} // end if
			} // end for

			np = lookup("AOSPATH",envVarHash);
			char path[100];
			std::copy(np->val->begin(),np->val->end(),path);
			pathTok = strtok(path,":");
			while (!loopDone && pathTok != NULL)
			{
				char *extCmd;
				if((extCmd = (char *)malloc(strlen(command)+strlen(pathTok)+strlen(cmdArray[1])+2)) != NULL)
				{
					extCmd[0] = '\0';   // empty string
					strcat(extCmd,command);
					strcat(extCmd,pathTok);
					strcat(extCmd,slash);
					strcat(extCmd,cmdArray[1]);
				} // end if
				system(extCmd);
				pathTok = strtok(NULL,":");
			} // end while
		} // end else if (strcmp(tok,witch) == 0)

		// if cmd is cd
   		// cd  (no args -> no change) (one arg may be relative or absolute)
		else if (strcmp(cmdArray[0],cd) == 0)
		{
			if (strcmp(cmdArray[1],"\0") == 0) 
			{ 
				syntax = "syntad: cd dir";
				//strcpy(syntax.c_str(),"syntax: cd dir");
				printf("%s\n",syntax.c_str()); 
				continue; 
			} // end if
			// TODO fix this a lot
			tok = cmdArray[1];
			tok = strtok(tok,"/");
			if (tok == NULL) 
			{	
				if (cmdArray[1][0]=='$') cmdArray[1] = interp(cmdArray[1]);
				chdir(cmdArray[1]);
				if (getcwd(cwd, sizeof(cwd)) != NULL) insert("AOSCWD",cwd,envVarHash);
			} // end if
			while (tok != NULL)
			{
				if (tok[0] =='$') tok = interp(++tok);
				chdir(tok);
				if (getcwd(cwd, sizeof(cwd)) != NULL)
				{
					insert("AOSCWD",cwd,envVarHash);
					printf("%s\n",lookup("AOSCWD",envVarHash)->val);
				} // end if
				tok = strtok(NULL,"/");
			} // end while (tok != NULL)
		} // end else if (strcmp(tok,cd) == 0)

		
		// if cmd is lim
   		// lim  arg1=time arg2=size
		else if (strcmp(cmdArray[0],lim) == 0)
		{
			char *time,*size;
			strcpy(time,cmdArray[1]);
			strcpy(size,cmdArray[2]);
			timeLim = strtol(time,NULL,10);

			// Dr butler removed requirement to set size limit

		} // end else if (strcmp(tok,lim) == 0)

		// if cmd is kill
		// kill num
		// Builtin command to cause the specified process to be sent the -KILL signal.
		else if(strcmp(cmdArray[0],kill)==0)
		{
			int i  = atoi(cmdArray[1]);
			sigkill(i);
		} // end else if (strcmp(tok,kill)==0)

		// if cmd is fg
		// fg num
		// Builtin command to cause the specified suspended or background process to become 
		// the foreground process.  In this context, the foreground process is merely the 
		// one which the shell is waiting for to complete.
		else if(strcmp(cmdArray[0],fg)==0)
		{
			int i  = atoi(cmdArray[1]);
			sigcontfg(i);
			waitpid(pid,&status,0);
		} // end else if (strcmp(tok,fg)==0)

		// if cmd is bg
		// bg num
		// Builtin command to cause the specified suspended process to run in the background.  
		// The shell will not wait for the process to complete because it is in the background.  
		// Of course, the shell may still perform a wait operation as part of handling a SIGCHLD 
		// for the process.		  
		else if(strcmp(cmdArray[0],bg)==0)
		{
			int i  = atoi(cmdArray[1]);
			sigcontbg(i);
		} // end else if (strcmp(tok,bg)==0)



		// if cmd is ext cmd
		else 
		{
			pid = 0;
			signal(SIGTSTP,sigstop);
			
			// look for |
			bool isPipe = false;
			bool amp = false;
			int pipeInd;
			char pipeStr[]="|";
			char ampStr[]="&";
			for (i=0;i<n_spaces-1;i++)
			{
				if (strcmp(pipeStr,cmdArray[i]) == 0)
				{
					isPipe = true;
					pipeInd = i;
				}
			}
			if (!isPipe)
			{
				// look for &
				for (i=0;i<n_spaces-1;i++)
				{
					if (strcmp(ampStr,cmdArray[i])==0)
					{
						--n_spaces;
						amp = true;
						cmdArray = (char **)realloc(cmdArray,sizeof(char *)*(n_spaces));
						cmdArray[n_spaces-1] = '\0';
					}
				} // end for

				if (!amp)
				{
					pid = fork();
					if (pid == 0) // child proc
					{
						setpgid(0,0);
						signal(SIGTSTP,SIG_DFL);
						childPid = getpid();
						printf("child pid: %d\n",getpid());
						rl.rlim_cur = rl.rlim_max = timeLim;
						setrlimit(RLIMIT_CPU,&rl);
						execvp(cmdArray[0],cmdArray);				
					}
					else if (pid != 0)
					{
						printf("parent pid: %d\n",getpid());
						//setpgid(pid,pid);
						waitpid(-1,&status,0);
						//wait(&status);
						//waitpid(-1,&status,WNOHANG);
					}
				}
				if (amp)
				{
					if ((pid = fork()) == 0) // child proc
					{
						int p = getpid();
						printf("[1] %d\n",p);
						signal(SIGTSTP,SIG_DFL);
						jobsVector.push_back(p);
						rl.rlim_cur = rl.rlim_max = timeLim;
						setrlimit(RLIMIT_CPU,&rl);
						execvp(cmdArray[0],cmdArray);				
						exit(0);
					}
					else
					{
						jobsVector.push_back(pid);
						setsid();
						usleep(100);
						signal(SIGCHLD,sigchld_handler);
					}
				} // end if (amp)
				amp = isPipe = false;
			} // end if (!isPipe)
			if (isPipe)
			{
				cmdArray[pipeInd] ='\0';
				int fd[2];
				pid_t pid1,pid2;
				pipe(fd);
				if ((pid1=fork())==0)
				{
					close(1);
					dup(fd[1]);
					close(fd[0]);
					close(fd[1]);
					rl.rlim_cur = rl.rlim_max = timeLim;
					setrlimit(RLIMIT_CPU,&rl);
					execvp(cmdArray[0],cmdArray);
					exit(0);
				} // end if
				if ((pid2=fork())==0)
				{
					close(0);
					dup(fd[0]);
					close(fd[0]);
					close(fd[1]);
					rl.rlim_cur = rl.rlim_max = timeLim;
					setrlimit(RLIMIT_CPU,&rl);
					execvp(cmdArray[pipeInd+1],&cmdArray[pipeInd+1]);
					close(0);
				} // end if
				close(fd[0]);
				close(fd[1]);
				waitpid(pid1,NULL,0);
				waitpid(pid2,NULL,0);
			} // end if (isPipe)
		} // end else
		tok = NULL;
	} // end while(!done)
	return 0;
} // end main



/*
                       Advanced Operating Systems
                                Fall 2016


turnin id:  aos_p3


Enhance the shell from p2 to support the new facilities described below.
 
 
 
Enhance the shell's capabilities with new support for handling signals so
that you can provide some job control.  Assume that there will never be
more than 4 processes to manage (foreground, background, suspended).
Also, for simplicity, assume that no task placed into the background will
attempt to read from stdin.

    ^Z on fg process suspends it
        Cause the foreground process to be suspended.

    jobs
        Builtin command to list current jobs (suspended or background).
        List each with a num beside it to be used in following commands.

    kill num
        Builtin command to cause the specified process to be sent
        the -KILL signal.

    fg num
        Builtin command to cause the specified suspended or background
        process to become the foreground process.  In this context, the
        foreground process is merely the one which the shell is waiting
        for to complete.

    bg num
        Builtin command to cause the specified suspended process to
        run in the background.  The shell will not wait for the process
        to complete because it is in the background.  Of course, the
        shell may still perform a wait operation as part of handling a
        SIGCHLD for the process.


Submitting the project for grading:

The project should compile and link as p3.

You should turnin a tar file containing all of the required files.

To build the project, I will cd to my directory containing your files
and simply type:

    rm -rf p3
    rm -rf *.o
    make

*/
