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
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tokenizer.h"

/* self build function */
int exe_program(struct tokens *tokens);
char* resolve_path(char *file);

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
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_wait(struct tokens *tokens);

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
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "prints the current working directory to standard output"},
  {cmd_cd, "cd", "changes the current working directory to that directory"},
  {cmd_wait, "wait", "waits until all background jobs have terminated before returning to the prompt"}
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/* Print the current working directory */
int cmd_pwd(unused struct tokens *tokens) {
  char buf[1024];
	getcwd(buf, 1024);
  printf("%s\n", buf);
  return 1;
}

/* changes the current working directory to that directory */
int cmd_cd(struct tokens *tokens) {
  char *directory2change = tokens_get_token(tokens, 1);
  if(directory2change == NULL) {
    printf("%s\n","Please type directory path");
  } else {
    chdir(directory2change);
  }
  return 1;
}

/* waits until all background jobs have terminated before returning to the prompt. */
int cmd_wait(struct tokens *tokens) {
  int status;
  pid_t pid;
  while(pid = wait(&status)) {
    if(pid == -1){
      printf("wait child error.\n");
      break;
    } else {
      printf("child #%d terminated.\n", pid);
      break;
    }
  }
  return 0;
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



int main(unused int argc, unused char *argv2exe[]) {
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
      /* REPLACE this to run commands as programs. */
      pid_t pid = fork();
      int status;

      int token_length = tokens_get_length(tokens);
      if (token_length <= 0)
      return 0;
      bool run_background = !strcmp(tokens_get_token(tokens, token_length - 1), "&");

      if(pid == 0) {
        status = exe_program(tokens);
        if(status < 0)
          printf("%d:%s\n", status, "Program execute error");
        exit(status);
      } else if(pid > 0) {
        if(!run_background){
          if(setpgid(pid, pid) == -1)
          printf("Child setpgid error!\n");

          /* When successful, tcsetpgrp() returns 0. Otherwise, it returns -1, and errno is set appropriately. */
          if(tcsetpgrp(shell_terminal, pid) == 0) {
            if ((waitpid(pid, &status, WUNTRACED)) < 0) {
              perror("wait failed");
              _exit(2);
            }

            if (WIFSTOPPED(status))
              printf("Process #%d stopped.\n", pid);
            if (WIFCONTINUED(status))
              printf("Process #%d continued.\n", pid);

            signal(SIGTTOU, SIG_IGN);
            if(tcsetpgrp(shell_terminal, shell_pgid) != 0)
              printf("switch to shell error occurred!\n");
            signal(SIGTTOU, SIG_DFL);

          } else {
            printf("tcsetpgrp error occurred\n");
          }
        }

      } else {
        printf("%s\n", "fork error");
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

int exe_program(struct tokens *tokens) {
  int token_length = tokens_get_length(tokens);
  if (token_length <= 0)
    return 0;

  char *file2redirect;

  char **argv2exe = (char **)calloc(token_length + 1, sizeof(char *));
  int argv_index = 0;
  for (int i = 0; i < token_length; ++i) {
    char *current_token = tokens_get_token(tokens, i);
    bool in_redirect = !strcmp(current_token, "<");
    bool out_redirect = !strcmp(current_token, ">");

    if(!(in_redirect || out_redirect)){
      if(strcmp(current_token, "&"))
        argv2exe[argv_index++] = current_token;
    } else {
      file2redirect = tokens_get_token(tokens, ++i);
      int redirect_fd, origin_fd;
      if(in_redirect){
        redirect_fd = open(file2redirect, O_RDWR);
        origin_fd = STDIN_FILENO;
      } else {
        // open(file2redirect, O_WRONLY | O_APPEND | O_CREAT, 0644); 不知道为什么并不会创建文件...
        redirect_fd = creat(file2redirect,  S_IRUSR | S_IWUSR |
                                            S_IRGRP | S_IWGRP |
                                            S_IROTH | S_IWOTH);
        origin_fd = STDOUT_FILENO;
      }

      if(redirect_fd == -1) {
        printf("open file error.\n");
        return -1;
      }
      int redirect_status = dup2(redirect_fd, origin_fd);
      if(redirect_status == -1) {
        printf("redirect standard out error.\n");
        return -1;
      }

      break;
    }
  }
  // Attentin! Must set the last to NULL or maybe can use char *[]
  argv2exe[argv_index] = (char *)NULL;

  char *complete_file_path = resolve_path(argv2exe[0]);
  int status = execv(complete_file_path, argv2exe);
  free(argv2exe);
  return status;
}

char* resolve_path(char *file) {
  char *PATH = getenv("PATH");
  char *file_path = (char *)calloc(1024, sizeof(char));
  char *current_path_ptr = PATH;

  while(true) {
    char *next_path_ptr = strchr(current_path_ptr, ':');
    if(!next_path_ptr) {
      next_path_ptr = strchr(current_path_ptr, '0');
    }
    size_t len = next_path_ptr - current_path_ptr;
    strncpy(file_path, current_path_ptr, len);
    file_path[len] = 0;

    strcat(file_path, "/");
    strcat(file_path, file);
    if(!access(file_path, F_OK)) {
      return file_path;
    }
    
    if(*next_path_ptr) {
      current_path_ptr = next_path_ptr + 1;
    } else {
      break;
    }
  }

  free(file_path);
  return NULL;
}