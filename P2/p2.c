// Name:		Ben Belden
// Class ID#:	bpb2v
// Section:		CSCI 6250-001
// Assignment:	Lab #2
// Due:			16:20:00, September 26, 2016
// Purpose:		Write a C/C++ program that implements a portion of my own shell.		
// Input:	    From terminal.	
// Outut:		To terminal.
// 
// File:		p1.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
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
typedef enum {false,true} bool;

// all hash functions from The C Programming Language p. 144-5
struct nlist {
	struct nlist *next;
	char *name;
	char *val;
};

/*struct rlimit {
	rlim_t rlim_curr;
	rlim_t rlim_max;
};*/

unsigned hash (char *s)
{
	unsigned hashval;
	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 + hashval;
	return hashval % HASHSIZE;
}

struct nlist *lookup(char *s, struct nlist *hashtab[HASHSIZE])
{
	struct nlist *np;
	for (np = hashtab[hash(s)]; np != NULL; np = np->next)
		if (strcmp(s,np->name) == 0)
			return np; // found
	return NULL; // not found
}

struct nlist *insert(char *name, char *val, struct nlist *hashtab[HASHSIZE])
{
	struct nlist *np;
	unsigned hashval;

	if ((np = lookup(name,hashtab)) == NULL) // not found 
	{
		np = (struct nlist *) malloc(sizeof(*np));
		if (np == NULL || (np->name = strdup(name)) == NULL)
			return NULL;
		hashval = hash(name);
		np->next = hashtab[hashval];
		hashtab[hashval] = np;
	}
	else // found
		free ((void *) np->val); // free previous val
	if ((np->val = strdup(val)) == NULL)
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

// TODO 
// A valid value on a command-line might be:
//    cd $HOME/$nextdir    ## assuming both vars have been defined
// where both HOME and nextdir must be interpolated.

// interpolate var
char * interp(char * s)
{
	s++;
	char * retStr;
	retStr = lookup(s,shellVarHash)->val;
	if (retStr == NULL)
		retStr = lookup(s,envVarHash)->val;
	return retStr; 
}

pid_t pid = 0;
void sigchld_handler(int s)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
	printf("\n[1]+  Done\t%d\n",pid);
}

int main (int argc, char *argv[])
{
	setbuf(stdout,NULL);
	clearenv();
	int i,rc,done=false,loopDone=false;
	char buff[100],*tok,*pathTok,*rest,*name,*val,syntax[]="syntax",nullTerm[]="\0";
	char cwd[1024],*buff_cpy;
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

	while (!done)
	{
		rc = getLine ("bpb2v_sh> ", buff, sizeof(buff));

		while (rc == NO_INPUT)
			rc = getLine ("bpb2v_sh> ", buff, sizeof(buff));

		// if cmd is exit or EOF or CTRL-D
		if (strcmp(buff,exitStr) == 0 || rc == EOFILE)
			break;
		 
		buff_cpy = malloc(sizeof(buff));
		strcpy(buff_cpy,buff);

		// if cmd is pwd
		if (strcmp(buff,pwd) == 0)
		{
			np = lookup("AOSCWD",envVarHash);
			printf("%s\n",np->val); 
		}

		// if cmd is envprt
		if (strcmp(buff,envprt) == 0)
			for(i=0;i<HASHSIZE;i++)
				if (envVarHash[i] != NULL)
					if (envVarHash[i]->val != NULL && envVarHash[i]->val[0] != '\0')
						printf("%s=%s\n",envVarHash[i]->name,envVarHash[i]->val);

		// if cmd is jobs
		// Builtin command to list current jobs (suspended or background).
		// List each with a num beside it to be used in following commands.
		if(strcmp(buff,jobs)==0)
		{
			;
		} // end if (strcmp(buff,jobs)==0)

		// break cmd into tokens
		tok = strtok(buff," ");

		//  if cmd is set
   		// set var val  (shell variable - not env)  - ‘ “ not required
		if (strcmp(tok,set) == 0)
		{
			strcpy(syntax,"syntax: set var val");
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: set var val");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			else name = tok;
			// check to see if name is valid
			if (!isValidName(name)) { printf("Invalid variable name.\n"); continue; }
			if (name[0] == '$') name = interp(name);
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: set var val");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			else val = tok;
			if (val[0] == '$') val = interp(val);
			insert(name,val,shellVarHash);
		} // end if (strcmp(tok,set) == 0)

		// if cmd is unset
   		// unset var    (shell variable - not env)
		else if (strcmp(tok,unset) == 0)
		{
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: unset var");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			else name = tok;
			// check to see if name is valid
			if (!isValidName(name)) { printf("Invalid variable name.\n"); continue; }
			if (name[0] == '$') name = interp(name);
			insert(name,nullTerm,shellVarHash);
		} // end else if (strcmp(tok,unset) == 0)

		// if cmd is prt
		// prt word $var (can have many args) 
		// (use shell's $var if same name in both shell and env)
		else if (strcmp(tok,prt) == 0)
		{
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: prt var1 var2 var3 ... varN");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			while (tok != NULL)
			{
				if (tok[0] == '$') 
				{	
					tok++;
					np = lookup(tok,shellVarHash);
					if (np == NULL || np->val[0] == '\0') np = lookup(tok,envVarHash);
					if (np != NULL && np->val[0] != '\0') printf("%s",np->val);
				} // end if
				else printf("%s ",tok);
				tok = strtok(NULL," ");
			} // end while
			printf("\n");
		} // end else if (strcmp(tok,prt) == 0)

		// if cmd is envset
   		// envset VAR VAL
		else if (strcmp(tok,envset) == 0)
		{
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: envset VAR VAL");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			else name = tok;
			if (name[0]=='$') name = interp(name);
			// check to see if name is valid
			if (!isValidName(name)) { printf("Invalid variable name.\n"); continue; }
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: envset VAR VAL");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			else val = tok;
			if (val[0]=='$') val=interp(val);
			insert(name,val,envVarHash);
			setenv(name,val,0);
		} // end else if (strcmp(tok,envset) == 0)

		// if cmd is envunset
   		// envunset VAR
		else if (strcmp(tok,envunset) == 0)
		{
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: envunset VAR");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			else name = tok;
			// check to see if name is valid
			if (!isValidName(name)) { printf("Invalid variable name.\n"); continue; }
			if (name[0]=='$') name=interp(name);
			insert(name,nullTerm,envVarHash);
			unsetenv(name);
		} // end else if (strcmp(tok,envunset) == 0)

		// if cmd is witch
   		// witch (like which :)  (looks thru AOSPATH for a cmd - see below)
		else if (strcmp(tok,witch) == 0)
		{
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: witch cmd");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			// check to see if built in
			loopDone=false;
			for (i=0;i<10 && !loopDone;i++)
			{
				if(strcmp(tok,cmds[i]) == 0)
				{
					printf("%s: shell built-in command.\n",tok);
					loopDone=true;
				} // end if
			} // end for

			np = lookup("AOSPATH",envVarHash);
			char path[100];
			strcpy(path,np->val);
			pathTok = strtok(path,":");
			while (pathTok != NULL)
			{
				char *extCmd;
				if((extCmd = malloc(strlen(command)+strlen(pathTok)+strlen(tok)+2)) != NULL)
				{
					extCmd[0] = '\0';   // empty string
					strcat(extCmd,command);
					strcat(extCmd,pathTok);
					strcat(extCmd,slash);
					strcat(extCmd,tok);
				} // end if
				system(extCmd);
				pathTok = strtok(NULL,":");
			} // end while
		} // end else if (strcmp(tok,witch) == 0)

		// if cmd is cd
   		// cd  (no args -> no change) (one arg may be relative or absolute)
		else if (strcmp(tok,cd) == 0)
		{
			tok = strtok(NULL," ");
			if (tok == NULL) 
			{ 
				strcpy(syntax,"syntax: cd dir");
				printf("%s\n",syntax); 
				continue; 
			} // end if
			name = tok;
			tok = strtok(NULL,"/");
			if (tok == NULL) 
			{	
				if (name[0]=='$') name = interp(name);
				chdir(name);
				if (getcwd(cwd, sizeof(cwd)) != NULL) insert("AOSCWD",cwd,envVarHash);
			} // end if
			while (tok != NULL)
			{
				if (tok[0]=='$') tok = interp(name);
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
		else if (strcmp(tok,lim) == 0)
		{
			char *time,*size;
			tok = strtok(NULL," ");
			strcpy(time,tok);
			tok = strtok(NULL," ");
			strcpy(size,tok);
			struct rlimit rl;
			// getrlimit(RLIMIT_CPU,&rl);
			rl.rlim_cur = strtol(time,NULL,10);
			setrlimit(RLIMIT_CPU,&rl);
			// getrlimit(RLIMIT_CPU,&rl);

			// Dr butler removed requirement to set size limit

		} // end else if (strcmp(tok,lim) == 0)

		// if cmd is kill
		// kill num
		// Builtin command to cause the specified process to be sent the -KILL signal.
		else if(strcmp(tok,kill)==0)
		{
			;
		} // end else if (strcmp(tok,kill)==0)

		// if cmd is fg
		// fg num
		// Builtin command to cause the specified suspended or background process to become 
		// the foreground process.  In this context, the foreground process is merely the 
		// one which the shell is waiting for to complete.
		else if(strcmp(tok,fg)==0)
		{
			;
		} // end else if (strcmp(tok,fg)==0)

		// if cmd is bg
		// bg num
		// Builtin command to cause the specified suspended process to run in the background.  
		// The shell will not wait for the process to complete because it is in the background.  
		// Of course, the shell may still perform a wait operation as part of handling a SIGCHLD 
		// for the process.		  
		else if(strcmp(tok,bg)==0)
		{
			;
		} // end else if (strcmp(tok,bg)==0)


		// if cmd is ext cmd
		else 
		{
			pid = 0;
			int status;
			// break input line into array of strings
			char **cmdArray = NULL;
			char *p = strtok(buff_cpy," ");
			int n_spaces = 0;
			while (p)
			{
				cmdArray = realloc(cmdArray,sizeof (char *)*(++n_spaces));
				if (cmdArray == NULL) break;
				cmdArray[n_spaces-1]=p;
				p = strtok(NULL," ");
			}
			cmdArray = realloc(cmdArray,sizeof(char *)*(++n_spaces));
			cmdArray[n_spaces-1]='\0';
			
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
						cmdArray = realloc(cmdArray,sizeof(char *)*(n_spaces));
						cmdArray[n_spaces-1] = '\0';
					}
				} // end for

				if (!amp)
				{
					pid = fork();
					if (pid == 0) // child proc
						execvp(cmdArray[0],cmdArray);				
					if (pid = waitpid(pid,&status,0)<0) // wait until child is done 
						// printf("waitpid error\n"); // TODO for testing only
				}
				if (amp)
				{
					if ((pid = fork()) == 0) // child proc
					{
						printf("[1] %d\n",getpid());
						execvp(cmdArray[0],cmdArray);				
						exit(0);
					}
					else
					{
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
					execvp(cmdArray[0],cmdArray);
					exit(0);
				} // end if
				if ((pid2=fork())==0)
				{
					close(0);
					dup(fd[0]);
					close(fd[0]);
					close(fd[1]);
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


turnin id:  aos_p2


Enhance the shell from p1 to support the new facilities described below.


-------- Commands Other Than Builtin --------
In addition to builtin commands, permit the user to execute commands
that are executable programs which can be located via AOSPATH.  If
there are multiple commands by the same name use the first one in the
path.  If a program happens to have the same name as a builtin command,
use the builtin one.  Note the user can skip use of the path and builtin
by providing a relative or full pathname.

Execution of programs should be done in a separate process created via
fork.  The exec system call should also be used in the separate process
to run the specified program.

By default, the shell should wait for an executed program to run.
However, if the user places an & at the end of the command, then the shell
should create the separate process to execute as background, and then
produce a new prompt for the user to do other things.  The shell should
print a useful message to the user when a background process terminates.

// TODO support cmd line args on non-builtins
// TODO support cmd options, no different than cmd line args

-------- I/O Redirection --------
The user should also be able to run processes in this manner:
    p1 | p2   ## which would cause stdout of p1 to go to p2's stdin
You can use pipe and dup/dup2 to assist in such an operation.

// TODO assume only one | 

We are NOT supporting < or > type operations at this time.


-------- Process Limits --------
Provide a new builtin command name "lim" which affects any processes
created thereafter.  lim takes either no args, or exactly 2 args.

    lim  ## no args; prints the current limits being used
    lim cputime memory  ## setrlimit CPU or AS
        lim 3 4  ## (max of 3 seconds cputime and 4 MB of memory)

// TODO use setrlimit between fork and exec on the child process


Submitting the project for grading:

The project should compile and link as p2.

You should turnin a tar file containing all of the required files.

To build the project, I will cd to my directory containing your files
and simply type:

    rm -rf p2
    rm -rf *.o
    make

*/
