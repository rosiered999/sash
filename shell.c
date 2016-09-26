#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/utsname.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<pwd.h>
#include<sys/wait.h>
#include<signal.h>

#define MAX_NO_OF_CMD_ELEMENTS (10)

struct utsname u; /* to get hostname*/
struct passwd *pw; /*to get username*/
int pid;
char* username, *hostname;
char cwd[100];
char *basedir;
int bg;

void init_prompt()
{
	getcwd(cwd,100);
	basedir = cwd;
	uname(&u);
	pw = getpwuid(getuid());
	username = pw->pw_name;
	hostname = u.nodename;
}


void prompt_me()
{
	sleep(1);
	getcwd(cwd,100);
	bg =0;
	printf("%s@%s:%s$ ",username,hostname,cwd);
}

void pinfo(char **argv)
{
	char path[1024];
	char c;
	char buf[1024];
	char filename[1000];
	printf("pid --- %s\n",argv[1]);
	sprintf(filename,"/proc/%s/status",argv[1]);
	FILE *f = fopen(filename, "r");
	FILE *fp;

	char state;
	fgets(buf,1024,f);
	fgets(buf,1024,f);
	sscanf(buf, "State: %c\n", &state);
	printf("process state = %c\n", state);
	fclose(f);
	char target_path[1024];
	sprintf(filename, "/proc/%d/exe",pid);
	int len = readlink (filename, target_path, sizeof (target_path));
	char buffer[1024];
	if(len ==-1)
	{
		perror("readlink");
	}
	else
	{
		target_path[len] = '\0';
		printf("executable path: %s\n", target_path);
	}
}

void pwd_me()
{
	getcwd(cwd,100);
	printf("%s",cwd); 
}

void cd_me(char **argv)
{
	chdir(argv[1]);
	if(getcwd(cwd,100)!=0)
	{
		perror(" ");
	}
	if(strcmp("~\0",argv[1])==0||strcmp("\0",argv[1])==0)
		chdir(basedir);
}

void echo_me(char **argv,int num)
{
	int i;
	for(i=0;i<num;i++)
	{
		printf("%s", argv[i]);
		if(i!=0 || i!=num-1)
		{
			printf(" ");
		}
	}
}

void execute(char **argv,int num)
{
	int i;
	pid_t  pid;
	int    status;

	if ((pid = fork()) < 0) 
	{     /* fork a child process*/
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	}
	else if (pid == 0) 
	{          /* for the child process: */
		if(strcmp(argv[0],"cd")==0)
			cd_me(argv);
		else if(strcmp(argv[0],"pwd")==0)
			pwd_me();
		else if(strcmp(argv[0],"echo")==0)
			echo_me(argv,num);
		else if(strcmp(argv[0],"pinfo")==0)
			pinfo(argv);
		else if(strcmp(argv[0],"exit")==0)
			return;
		int c;
		if (c==execvp(argv[0], argv) < 0) 
		{     /* execute the command  */
			printf("%d\n", c);
			printf("*** ERROR: exec failed\n");
			perror(" ");
			exit(1);
		}
	}
	else if(bg!=1){
		while (wait(&status) != pid);

	}
}

void input()
{
	int i,j,n,len,c;
	char *buffer = 0;
	size_t bufsize = 0;
	ssize_t characters;
	char *cmd[MAX_NO_OF_CMD_ELEMENTS+1];

	characters = getline(&buffer, &bufsize, stdin);

	len = strlen(buffer);
	buffer[len-1]='\0';

	if (characters > 0)
	{
		bg = 0;
		char *end_str1;
		char *token1 = strtok_r(buffer, ";", &end_str1);
		int count = 0, wordcnt;
		while (token1 != NULL)
		{
			char *token2;
			memset(cmd,0,sizeof(cmd));
			char * cmd[MAX_NO_OF_CMD_ELEMENTS + 1]; /* 1+ for the NULL-terminator */
			size_t wordcnt = 0;
			char *end_str2;
			count++;
			token2 = strtok_r(token1, " ", &end_str2);
			while ((NULL != token2)
					&& (MAX_NO_OF_CMD_ELEMENTS > wordcnt)) /* Prevent writing
															  out of `cmd`'s bounds. */
			{
				cmd[wordcnt] = token2;
				wordcnt++;
				token2 = strtok_r(NULL, " ", &end_str2);
			}
			if(token2==NULL)
			{
				if(strcmp(cmd[wordcnt-1],"&")==0)
				{
					bg = 1;
					cmd[wordcnt-1] = NULL;
					execute(cmd,wordcnt-1);
				}
				else
				{
					cmd[wordcnt] = NULL;
					execute(cmd, wordcnt);
				}
			}
			token1 = strtok_r(NULL, ";", &end_str1);
		}
	}
	free(buffer);
}


int main()
{
	init_prompt();
	while(1)
	{
		prompt_me();
		input();
	}
}
