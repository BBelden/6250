// Name:		Ben Belden
// Class ID#:	bpb2v
// Section:		CSCI 6250-001
// Assignment:	Lab #1
// Due:			16:20:00, September 7, 2016
// Purpose:		Write a C/C++ program that implements a portion of my own shell.		
// Input:	    From terminal.	
// Outut:		To terminal.
// 
// File:		p1.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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


int main (int argc, char *argv[])
{
	int i,rc,done=false,loopDone=false;
	char buff[100],*tok,*pathTok,*rest,*name,*val,syntax[]="syntax",nullTerm[]="\0";
	char cwd[1024];
	const char command[]="which ",slash[]="/",exit[]="exit",set[] = "set";
	const char unset[]="unset",prt[]="prt",envset[]="envset",envunset[]="envunset";
	const char envprt[]="envprt",witch[]="witch",pwd[]="pwd",cd[]="cd";
	const char *cmds[10];
	cmds[0]="exit"; cmds[1]="set"; cmds[2]="unset"; cmds[3]="prt"; cmds[4]="envset";
	cmds[5]="envunset"; cmds[6]="envprt"; cmds[7]="witch"; cmds[8]="pwd"; cmds[9]="cd";
	struct nlist *np;
	insert("AOSPATH","/bin:/usr/bin",envVarHash);
	if (getcwd(cwd, sizeof(cwd)) != NULL)
		insert("AOSCWD",cwd,envVarHash);

	while (!done)
	{
		rc = getLine ("bpb2v_sh> ", buff, sizeof(buff));

		while (rc == NO_INPUT)
			rc = getLine ("bpb2v_sh> ", buff, sizeof(buff));

		// if cmd is exit or EOF or CTRL-D
		if (strcmp(buff,exit) == 0 || rc == EOFILE)
			break;

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
			}
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
			}
			else val = tok;
			if (val[0] == '$') val = interp(val);
			insert(name,val,shellVarHash);
		}

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
			}
			else name = tok;
			// check to see if name is valid
			if (!isValidName(name)) { printf("Invalid variable name.\n"); continue; }
			if (name[0] == '$') name = interp(name);
			insert(name,nullTerm,shellVarHash);
		}

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
			}
			while (tok != NULL)
			{
				if (tok[0] == '$') tok++;
				// lookup in shell vars
				np = lookup(tok,shellVarHash);
				// if not there, lookup in env vars
				if (np == NULL || np->val[0] == '\0') np = lookup(tok,envVarHash);
				else printf("%s=%s\n",tok,np->val);
				if (np == NULL || np->val[0] == '\0') printf("");
				else printf("%s=%s\n",tok,np->val);
				tok = strtok(NULL," ");
			}
		}

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
			}
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
			}
			else val = tok;
			if (val[0]=='$') val=interp(val);
			insert(name,val,envVarHash);
		}

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
			}
			else name = tok;
			// check to see if name is valid
			if (!isValidName(name)) { printf("Invalid variable name.\n"); continue; }
			if (name[0]=='$') name=interp(name);
			insert(name,nullTerm,envVarHash);
		}

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
			}
			// check to see if built in
			loopDone=false;
			for (i=0;i<10 && !loopDone;i++)
				if(strcmp(tok,cmds[i]) == 0)
				{
					printf("%s: shell built-in command.\n",tok);
					loopDone=true;
				}

			np = lookup("AOSPATH",envVarHash);
			pathTok = strtok(np->val,":");
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
				}
				system(extCmd);
				printf("%s\n",extCmd);
				pathTok = strtok(NULL,":");
			}
		}

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
			}
			name = tok;
			tok = strtok(NULL,"/");
			if (tok == NULL) 
			{	
				if (name[0]=='$') name = interp(name);
				chdir(name);
				if (getcwd(cwd, sizeof(cwd)) != NULL) insert("AOSCWD",cwd,envVarHash);
			}
			while (tok != NULL)
			{
				if (tok[0]=='$') tok = interp(name);
				chdir(tok);
				if (getcwd(cwd, sizeof(cwd)) != NULL)
				{
					insert("AOSCWD",cwd,envVarHash);
					printf("%s\n",lookup("AOSCWD",envVarHash)->val);
				}
				tok = strtok(NULL,"/");
			}
		}
		tok = NULL;
	}
	return 0;
}


/*

Write a C/C++ program that implements a portion your own shell.  It should
print a prompt which is the concatenation of your login id and the char
string "_sh> " (note the blank at the end). So, if I were to implement
the project, the prompt I would print would be "rbutler_sh> " without
the quotes.  However, the shell should not print a prompt if its stdin
is not a tty or if it is given a single command-line arg which is a
file to accept commands from instead.  It is valid to assume that no
command-line will be more that 64 bytes.

For ease of parsing, you may assume that all command-line values are
separated by at least one char of white-space and that a single command
is on a single line.  A # anywhere on the line marks the beginning of
a comment and thus the end of the line.

The shell should provide these builtin commands:
   ***exit (or end-of-file, e.g. cntl-D on tty)
   ***set var val  (shell variable - not env)  - ‘ “ not required
   ***unset var    (shell variable - not env)
   prt word $var  (use shell's $var if same name in both shell and en
   envset VAR VAL
   ***envunset VAR
   ***envprt (print the entire environment)
   ***witch (like which :)  (looks thru AOSPATH for a cmd - see below)
   ***pwd
   cd  (no args -> no change) (one arg may be relative or absolute)

Builtin commands are executed in the shell's process.  Variables can
be used on any command-line, but no variable value can contain a $
that might cause recursive variable interpolation.  Shell variables
are to be used before env vars if both exist unless otherwise stated.

A valid value on a command-line might be:
    cd $HOME/$nextdir    ## assuming both vars have been defined
where both HOME and nextdir must be interpolated.
Valid vars consist of alphas, numerics, and underscores, beginning with
alpha or underscore.

The shell should provide these environment variables at start-up:
     AOSPATH (default is /bin:/usr/bin)
     AOSCWD  (current working directory) (cd should cause it to be updated)
And, those two vars should be the ONLY two at start-up.
So, I should be able to immediately execute:
     prt $AOSPATH $AOSCWD

For this project, you do NOT have to execute any commands other than those
built into the shell.


Submitting the project for grading:

The project should compile and link as p1.

You should turnin a tar file containing all of the required files.

To build the project, I will cd to my directory containing your files
and simply type:

    rm -rf p1 *.o
    make
*/

