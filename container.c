#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#define STACKSIZE (1024 * 1024)

struct params{

  char * cmd;
  char ** argv;
  int argc;
  int cpu_pct;
  int mem_limit;
  int num_levels;

};


static char stack[STACKSIZE];

void print_err(char const * const reason)
{
    fprintf(stderr, "Error %s: %s\n", reason, strerror(errno));
}
int exec(void * args)
{

  // Validate passing of struct
  struct params * userParams = ((struct params *)args);

  printf("cmd: %s\n", userParams->cmd);
  printf("cpu_pct: %d\n", userParams->cpu_pct);
  printf("mem_limit: %d\n", userParams->mem_limit);
  printf("num_levels: %d\n", userParams->num_levels);

  for (int i = 0; i < userParams->argc; i++){
    printf("Arg #%d: %s\n", i, userParams->argv[i]);
  }

    // Remount proc
    // Use print_err to print error if mount fails

  int result;
  result = mount("proc", "/proc", "proc", 0, NULL);
  if (result != 0){
    print_err("mount");
  }

    char const * const hostname = "CSC595";
    // Set a new hostname
    // Use print_err to print error if sethostname fails
    result = sethostname(hostname, strlen(hostname));

    if (result != 0){
      print_err("sethostname");
    }

    // Create a message queue
    // USe print_err to print error if creating message queue fails
    result = msgget(0, IPC_CREAT | 0666);
    if (result < 0){
      print_err("msgget");
    }


    // Execute the given command
    // Use print_err to print error if execvp fails
    result = execvp(userParams->cmd, userParams->argv);
    if (result != 0){
      print_err("execvp");
    }

    return 0;
}

int main(int argc, char ** argv)
{
    // Provide some feedback about the usage
    if (argc < 2) {
        fprintf(stderr, "USAGE ./container num_namespaces cmd cpu_pct mem_limit num_levels [...args]\n");
        return 1;
    }

    int numNamespaces = atoi(argv[1]);

    // Initialize params struct
    struct params * userParams = (struct params *)malloc(sizeof(struct params));
    userParams->cmd = (char *)malloc(strlen(argv[2]) + 1);
    strcpy(userParams->cmd, argv[2]);



    userParams->cpu_pct = atoi(argv[3]);
    userParams->mem_limit = atoi(argv[4]);
    userParams->num_levels = atoi(argv[5]);

    // Fill in additional args
    userParams->argc = 0;
    int numArgs = argc - 6;
    if (numArgs > 0){
      userParams->argv = (char **)malloc((sizeof(char *) * numArgs) + 1);
      userParams->argc = numArgs;
      userParams->argv[numArgs] = NULL;
    }
    else{
      userParams->argv = (char **)malloc(sizeof(char *) * 2);
      userParams->argv[0] = "";
      userParams->argv[1] = NULL;
    }

    for (int i = 6; i < argc; i++){
      userParams->argv[i-6] = (char *)malloc(strlen(argv[i]) + 1);
      strcpy(userParams->argv[i - 6], argv[i]);
    }

    // Namespace flags
    // ADD more flags
    const int flags =
        SIGCHLD
      | CLONE_NEWNS
      | CLONE_NEWPID
      | CLONE_NEWNET
      | CLONE_NEWIPC
      | CLONE_NEWUTS
      | CLONE_NEWUSER;

    pid_t pid;
    pid_t * childPIDs = (pid_t *)malloc(sizeof(pid_t) * numNamespaces);
    int status;

    for(int i = 0; i < numNamespaces; i++)
      {
		switch(childPIDs[i] = fork())
          {
          case -1:
			{
              print_err("clone");
              return -1;
			}
          case 0:
			{
              printf("%d, child\n", i);
              pid = clone(exec, stack + STACKSIZE, flags, (void *)userParams);

              while(waitpid(pid, &status, 0) < 0 && errno == EINTR)
			  {
                continue;
              }

              if (WIFEXITED(status))
			  {
                return WEXITSTATUS(status);
              }

              _exit(EXIT_SUCCESS);
			}
          default:
			{
              printf("%d parent\n", i);
              if (wait(NULL) == -1){
                print_err("waiting for pid");
                return 1;
              }
              break;
			}
          }
      }

    // Return the exit code
    return WEXITSTATUS(status);
}
