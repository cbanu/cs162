#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <linux/limits.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_help(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_cd, "cd", "change working directory"},
  {cmd_pwd, "pwd", "print current working directory"},
  {cmd_exit, "exit", "exit the command shell"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Changes working directory */
int cmd_cd(unused struct tokens *tokens) {
  if (2 != tokens_get_length(tokens)) {
    fprintf(stderr, "usage: cd <dir>\n");
    return 1;
  }
  if (0 != chdir(tokens_get_token(tokens, 1))) {
    fprintf(stderr, "error: %s\n", strerror(errno));
    return 1;
  }
  return 0;
}

/* Prints working directory */
int cmd_pwd(unused struct tokens *tokens) {
  if (1 != tokens_get_length(tokens)) {
    fprintf(stderr, "usage: pwd\n");
    return 1;
  }
  char wd[PATH_MAX];
  if (NULL == getcwd(wd, PATH_MAX)) {
    fprintf(stderr, "error: %s\n", strerror(errno));
    return 1;
  }
  printf("%s\n", wd);
  return 0;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

/* return the complete path to a corresponding executable file */
/* note: the returned pointer must be deallocated */
char *get_exe_file(const char *cmd) {
  if (cmd == NULL) {
    return NULL;
  }
  if (NULL != strchr(cmd, '/')) {
    if (access(cmd, X_OK) != -1) {
      return strdup(cmd);
    } else {
      return NULL;
    }
  } else {
    char *path = getenv("PATH");
    if (path == NULL) {
      return NULL;
    }
    /* duplicate the string so that we don't change the process' environment */
    path = strdup(path);
    {
      /* tokenize the PATH */
      char *srchptr, *saveptr, *toknptr;
      for (srchptr = path; NULL != (toknptr = strtok_r(srchptr, ":", &saveptr)); srchptr = NULL) {
        /* compose a combined path string (make room for '/' and terminating NULL */
        char *fullcmd = malloc(strlen(toknptr) + 1 + strlen(cmd) + 1);
        sprintf(fullcmd, "%s/%s", toknptr, cmd);
        /* check if such a file exists and is executable */
        if (access(fullcmd, X_OK) != -1) {
          /* free path duplicate, and return allocated executable path */
          free(path);
          return fullcmd;
        } else {
          /* don't forget to free this, not needed for next search */
          free(fullcmd);
        }
      }
    }
    /* no suitable executable file found */
    free(path);
    return NULL;
  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      char *cmd = tokens_get_token(tokens, 0);
      char *exe = get_exe_file(cmd);
      if (exe != NULL) {
        /* file exists and it's executable, prepare data for execv() function */
        size_t narg = tokens_get_length(tokens);
        char **args = malloc((narg + 1) * sizeof(char*));
        if (NULL == args) {
          fprintf(stderr, "error: %s\n", strerror(errno));
          exit(1);
        }
        int i;
        for (i = 0; i < narg; i++) {
          args[i] = tokens_get_token(tokens, i);
        }
        args[narg] = NULL;
        pid_t pid = fork();
        if (pid == 0) {
          /* child */
          /* execute command */
          if (-1 == execv(exe, args)) {
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
          }
        } else {
          /* parent */
          /* wait for any process to end, it's only one */
          int status;
          wait(&status);
          /* free dynamically allocated data in the parent process */
          free(exe);
          free(args);
        }
      } else {
        if (cmd != NULL) {
          fprintf(stdout, "Unknown command '%s'.\n", cmd);
        }
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
