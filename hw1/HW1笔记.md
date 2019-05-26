# HW1: Basic Shell

## 1. Getting started

第一个小任务只是需要下载下hw文件，先试一下已经做了一部分的 shell 程序。虽然上面让用 Vagrant Virtual Machine 进行操作，但虚拟机运行太慢而且容易出 bug，直接在 Linux 上跑就可以。

[hw下载地址](<https://github.com/Berkeley-CS162/ta>)

[hw1.pdf](<https://inst.eecs.berkeley.edu/~cs162/fa15/static/hw/hw1.pdf>)

## 2. Add support for cd and pwd

```
Add a new built-in pwd that prints the current working directory to standard output. Then, add a new built-in cd that takes one argument, a directory name, and changes the current working directory to that directory.
```

第二个小任务是给 shell 程序添加两个内置命令，一个是 **cd**， 一个是 **pwd**。

cd 命令用于改变当前的工作目录， pwd 命令用于将当前工作目录改成输入的目录。

### 实现方法：照猫画虎

最初的 shell 程序已经实现了内置命令 **quit** 和 **?**，

main 函数中的主要相关代码如下：

```c
	* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));
	if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      fprintf(stdout, "This shell doesn't know how to run programs.\n");
    }
```

main 函数中的这段代码说明 shell 执行内置命令时就是通过执行内置命令数组中对应的函数。

```c
/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
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
```

通过观察内置命令数据结构的代码可以发现，**cmd_table** 数组中定义的是命令的说明,**cmd_help** 和 **cmd_exit** 定义的是命令的操作，我们可以先实现 cd 和 pwd 的操作。

```c
/* Print the current working directory */
int cmd_pwd(unused struct tokens *tokens) {
  char buf[1024];
	getcwd(buf, 1024);
  printf("%s\n", buf);
}

/* changes the current working directory to that directory */
int cmd_cd(struct tokens *tokens) {
  char *directory2change = tokens_get_token(tokens, 1);
  if(directory2change==NULL) {
    printf("%s\n","Please type directory path");
  } else {
    chdir(directory2change);
  }
}
```

然后在 **cmd_table** 中将函数和命令名绑定即可，这个任务比较简单。

```c
fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "prints the current working directory to standard output"},
  {cmd_cd, "cd", "changes the current working directory to that directory"},
};
```



## 3. Execute programs

```
Modify your shell so that, when it receives a command that isn’t a known built-in, it forks a child process to execute the command as a program. The first word of the command should be the program’s name. The rest of the words are the command-line arguments to the program.
For this step, you can assume that the first word of the command will be the full path to the program. So instead of running wc, you would run /usr/bin/wc. In the next section, you will implement support for looking up program names like wc. But you can pass some tests without supporting that feature.
```

第三个任务是完成用户在 shell 中输入非内置命令时执行输入的程序，程序必须是绝对路径，比如 **/usr/bin/wc shell.c**.

### 实现方法：查看 exec 函数的使用

hw1 中对于这个功能的 implement 给了提示——先 fork 一个子进程，然后调用一个 **exec** 函数的变体来执行程序。

exec 是一组函数，包括下面 6 个函数：

```c
	int execl(const char *path, const char *arg, ...);
 
　　int execlp(const char *file, const char *arg, ...);
 
　　int execle(const char *path, const char *arg, ..., char *const envp[]);
 
　　int execv(const char *path, char *const argv[]);
 
　　int execvp(const char *file, char *const argv[]);
 
　　int execve(const char *path, char *const argv[], char *const envp[]);
```

只要在 Google 中看一下 **man exec** 的说明，最后可以发现我们应该使用的是 **execv** 函数。

实现代码如下：

```c
int exe_program(struct tokens *tokens) {
  int token_length = tokens_get_length(tokens);
  if (token_length <= 0) {
    return 0;
  }

  char **argv2exe = (char **)calloc(token_length, sizeof(char *));
  for (int i = 0; i < token_length; ++i) {
    argv2exe[i] = tokens_get_token(tokens, i);
  }

  for (int i = 0; i < token_length; ++i) {
    printf("%s\n", argv2exe[i]);
  }

  int status = execv(argv2exe[0], argv2exe);
  free(argv2exe);
  return status;
}
```

execv 和 execvp 函数的不同之处就在于 **execv 函数必须输入程序的全路径，而 execvp 函数会自动解析路径，**因此第四个任务 resolve path 不允许使用 execvp 函数。



## 4. Path resolution

```shell
When a shell executes a program like wc, it looks for that program in each directory on the path and executes the first one that it finds. The directories on the path are separated with a colon. This process is called resolving the path.
Modify your shell so that it reads the PATH variable from the environment and uses it to resolve program names in the path. Typing in the full pathname of the executable should still be supported.
```

实现 shell 自动读取 PATH，使用 PATH 变量解析输入的程序名，输入全路径的程序仍然可以执行，禁止使用 **execvp** 函数。

### 实现：

将程序名和每个 PATH 变量结合后查看文件是否存在，存在就可以执行程序，主要是查看 **getenv** 和 **access** 函数的使用。

```c
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
```

## 5.Input/Output Redirection

```
When running programs, it is sometimes useful to be able to feed in input from a file or to write the output to a file. The syntax [process] > [file] specifies to write the process’s output to the file. Similarly, the syntax [process] < [file] specifies to feed in the contents of the file into the process. The commands dup2 and strcmp may be useful here.
Modfiy your shell so that it supports redirecting stdin and stdout to files. You do not need to support redirection for shell built-in commands. You do not need to support stderr redirection or appending to files (e.g. [process] >> [file]). You can assume that there will always be spaces around special characters < and >.
```

只实现 **>** 和 **<** 的输入输出重定向，命令格式为 **your_program > file_name** 之类，将程序输出写入文件，或者 **your_program < file_name** ，将文件作为程序的输入。

### 实现：

在之前 **exec_program** 函数中重定向输入输出流。

```c
  char *file2redirect;

  char **argv2exe = (char **)calloc(token_length + 1, sizeof(char *));
  int argv_index = 0;
  for (int i = 0; i < token_length; ++i) {
    char *current_token = tokens_get_token(tokens, i);
    bool in_redirect = !strcmp(current_token, "<");
    bool out_redirect = !strcmp(current_token, ">");

    if(!(in_redirect || out_redirect)){
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
```

和作业里说的一样，**dup2** 和 **strcmp** 函数非常关键，搞懂他们的用法就没问题了。

## 6. Signal Handling and Terminal Control

```
Shells let you interrupt or pause processes with special key strokes. For example, pressing CTRL-C sends SIGINT which stops the current program, while pressing CTRL-Z sends SIGTSTP which sends the current program to the background. Right now, these signals are sent directly to our shell. This is not what we want, since for example attempting to CTRL-Z a process will also stop our shell. We want to have the signals affect the processes that our shell creates, not the shell itself.
```

以往我们在自己的 shell 程序中 按 CTRL-C 时会杀掉整个 shell 程序，而不是 shell 中我们通过命令调用的程序，这次我们需要通过 Signal  Handling 将 CTRL-C 产生的中断信号传给 shell 中运行的程序。

### Suggested implementation:

```
You can use the signal function to change how signals are handled by the current process. The shell should basically ignore most of these signals, whereas the programs it runs should do the default action. Beware: forked processes will inherit the signal handlers of the original process. Reading man 2 signal and man 7 signal will provide more information. For more information about how signals work, please
work through the tutorial here.
You should ensure that each program you start is in its own process group. When you start a process, its process group should be placed in the foreground. You can use the function we provided, put_process_in_foreground, to do this, or you can call tcsetpgrp directly.
```

题目后面也给了对于实现的相关知识介绍，比如 **Process group、Foreground terminal、Signal**  ，最后提示了 **signal** 和 **tcsetpgrp** 函数的使用。需要注意的是，题目只要求可以在 shell 中控制程序，没要求再使用 **fg** 等命令可以恢复进程。

USNA 的 IC221 的 lab10 的要求和这次作业很像，[lab10](<https://www.usna.edu/Users/cs/aviv/classes/ic221/s16/lab/10/lab.html>) 对于 **signal** 和 **tcsetpgrp** 函数的使用进行了详细的入门介绍，再结合 man signal 和 man tcsetpgrp 基本可以完成这个作业。

**我的实现**：

```c
	if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      pid_t pid = fork();
      int status;

      if(pid == 0) {
        status = exe_program(tokens);
        if(status < 0)
          printf("%d:%s\n", status, "Program execute error");
        exit(status);
      } else if(pid > 0) {
        if(setpgid(pid, pid) == -1) {
          printf("Child setpgid error!\n");
        }

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
          if(tcsetpgrp(shell_terminal, shell_pgid) != 0) {
            printf("switch to shell error occurred!\n");
          }
          signal(SIGTTOU, SIG_DFL);

        } else {
          printf("tcsetpgrp error occurred\n");
        }

      } else {
        printf("%s\n", "fork error");
      }
    }
```

## 7.Background processing

```
Modify your shell so that it runs commands that end in an “&” in the background. You only need to support backgrounding for programs, not built-in commands.
You should also add a new built-in command wait, which waits until all background jobs have terminated before returning to the prompt.
You can assume that there will always be spaces around the & character. You can assume that, if there is a & character, it will be the last token on that line.
```

实现在命令最后加 **&** 使输入的程序后台执行，和实现内置命令 **wait**，输入 **wait** 后会直到后台程序全部执行完才返回输入命令行。

第一个 & 后台执行很简单，既然是后台执行，直接让父进程直接运行，不再 wait 子进程即可。

第二个 wait 的实现也很简单，既然是等待所有后台程序执行，直接让父进程一直等待所有子进程即可。

### 实现：

```c
// & Background Processing

// 先判断是不是后台执行的程序
bool run_background = !strcmp(tokens_get_token(tokens, token_length - 1), "&");

else if(pid > 0) {
    //如果不是后台程序才和之前一样等待，否则直接pass
        if(!run_background){
          if(setpgid(pid, pid) == -1)
          printf("Child setpgid error!\n");

          /* When successful, tcsetpgrp() returns 0. Otherwise, it returns -1, and errno is set appropriately. */
          if(tcsetpgrp(shell_terminal, pid) == 0) {
            if ((waitpid(pid, &status, WUNTRACED)) < 0) {
```

```c
// wait 函数
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
// 剩下的就是和第一个任务一样加到 cmd_table 之类的
```



## 感悟

虽然回头看来这些任务都不是特别难，但刚开始用 C 写的时候还是很艰难的，不但要好好看各种底层函数的手册，出了 bug 还不知道怎么调试。不过通过完成这些任务，对于操作系统的 Signal 和 Thread 等都有了更深的理解，Kubi 教授课上讲的很清晰，而且也很生动，但只有自己试着码一些东西才能有所体会。有点遗憾的可能就是这学期估计只能完成 CS162 的 Homework 了，安排的小组作业的各个 Project 难度比较大，而且这学期剩下的时间也不多了，希望有想跟 CS162 的同学感兴趣的话可以暑假一块组队做一下那三个 Project。

[作业源码](<https://github.com/DateBro/CS162-Homework>)