#include<stdio.h>
#include<sys/resource.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/utsname.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<pwd.h>
#include<sys/wait.h>
#include<signal.h>
#include<sys/stat.h>
#include<fcntl.h>

#define MAX_NO_OF_CMD_ELEMENTS (10)

int exit1 = 0;

struct jobs{
    int num;
    int pid;
    int status;
    char *name;
};

struct jobs jobList[50];
int jobNo = 0;
struct utsname u; /* to get hostname*/
struct passwd *pw; /*to get username*/
int pid;
char* username, *hostname;
char cwd[100];
char basedir[100];
int bg;

void init_prompt()
{
    getcwd(cwd,100);
    strcpy(basedir,cwd);
    uname(&u);
    pw = getpwuid(getuid());
    username = pw->pw_name;
    hostname = u.nodename;
}

void listjobs()
{
    int i;
    char proc_pid[100];
    for(i=1;i<=jobNo;i++)
    {
        sprintf(proc_pid, "/proc/%d", jobList[i].pid);
        struct stat sts;
        if(stat(proc_pid, &sts) == -1 && errno == ENOENT);
        else
            printf("[%d] %s {%d}\n", i,jobList[i].name,jobList[i].pid);
    }
}

void prompt_me()
{
    getcwd(cwd,100);
    bg =0;
    printf("%s@%s:%s$ ",username,hostname,cwd);
}


int pinfo(int argc, char * argv[]) {
    char filename[1000];
    printf("pid --- %s\n",argv[1]);
    sprintf(filename,"/proc/%s/status",argv[1]);
    FILE *f = fopen(filename, "r");
    FILE *fp;

    char state,buf[1024];
    fgets(buf,1024,f);
    fgets(buf,1024,f);
    sscanf(buf, "State: %c\n", &state);
    printf("process state = %c\n", state);
    fclose(f);
    char target_path[1024];
    sprintf(filename, "/proc/%s/exe",argv[1]);
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
    return 0;
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
        ;
    }
    if(strcmp("~",argv[1])==0||strcmp("",argv[1])==0)
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
    char *name;

    if ((pid = fork()) < 0) 
    {     /* fork a child process*/
        printf("*** ERROR: forking child process failed\n");
        exit(1);
    }
    else if (pid == 0) 
    { 
        for(i=0;i<num;i++)
        {
            if(strcmp(argv[i],"<")==0)
            {
                name = strdup(argv[i+1]);
                int fin = open(name, O_RDONLY,0);
                dup2(fin,0); //put error
                close(fin);
                argv[i]=NULL;
                fflush(stdin);
            }
            else if(strcmp(argv[i], ">")==0)
            {
                name = strdup(argv[i+1]);
                int fout = creat(argv[i+1],0644);
                dup2(fout,1);
                close(fout);
                argv[i] = NULL;
            }
            else if(strcmp(argv[i], ">>")==0)
            {
                name = strdup(argv[i+1]);
                int fout = open(name, O_RDWR | O_APPEND);
                dup2(fout,1);
                close(fout);
                argv[i] = NULL;
            }
        }
        if(strcmp(argv[0],"cd")==0)
            cd_me(argv);
        else if(strcmp(argv[0],"pwd")==0)
            pwd_me();
        else if(strcmp(argv[0],"echo")==0)
            echo_me(argv,num);
        else if(strcmp(argv[0],"pinfo")==0)
            pinfo(num,argv);
        else if(strcmp(argv[0], "listjobs")==0)
            listjobs();
        else if(strcmp(argv[0],"quit")==0)
        {
            exit1 = 1;
            return;
        }
        /* for the child process: */
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
        jobNo++;
        jobList[jobNo].name = strdup(argv[0]);
        jobList[jobNo].num = jobNo;
        jobList[jobNo].status = 0;
        while (wait(&status) != pid);
    }
    else if(bg==1)
    {
        jobNo++;
        jobList[jobNo].name = strdup(argv[0]);
        jobList[jobNo].num = jobNo;
        jobList[jobNo].status = 1;
    }

    jobList[jobNo].pid = pid;
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

void sig_handler(int signo)
{
   // if(signo==SIGINT);
    if(signo==SIGTSTP);
}


int main()
{
    if(signal(SIGINT, sig_handler)==SIG_ERR)
        write(1,"\ncan't catch SIGINT\n", 50);
    if(signal(SIGTSTP, sig_handler)==SIG_ERR)
        system("echo '\ncan't catch SIGTSTOP\n'");
    init_prompt();
    while(exit1!=1)
    {
        prompt_me();
        input();
    }
    return 0;
}
