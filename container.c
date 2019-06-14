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

// #include "pivot_root.h"
int my_pivot_root();

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

    my_pivot_root();

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
      fprintf(stderr, "USAGE ./container cmd\n");
      return 1;
    }

    // Initialize params struct
    struct params * userParams = (struct params *)malloc(sizeof(struct params));

    size_t len = strlen(argv[1]);
    userParams->cmd = (char *)malloc(len+1);
    strcpy(userParams->cmd, argv[1]);
    userParams->cmd[len] = '\0';

    userParams->argv = (char **)malloc(sizeof(char *) * 3);
    userParams->argv[0] = (char *)malloc(len+1);
    strcpy(userParams->argv[0], argv[1]);
    userParams->argv[0][len] = '\0';
    userParams->argv[1] = "";
    userParams->argv[2] = NULL;

    // Namespace flags
    const int flags =
        SIGCHLD
      | CLONE_NEWNS
      | CLONE_NEWPID
      | CLONE_NEWNET
      | CLONE_NEWIPC
      | CLONE_NEWUTS
      | CLONE_NEWUSER;

    pid_t pid;
    int status;

    pid = clone(exec, stack + STACKSIZE, flags, (void *)userParams);
    waitpid(pid, &status, 0);

    return 0;
}
