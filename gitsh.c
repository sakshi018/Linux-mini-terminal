#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
 
/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
 
/*
  List of builtin commands, followed by their corresponding functions.
 */
 #define BUILTIN_COMMAND_COUNT 3
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};
 
#define RANDOMIZE_FACTOR_MESSAGE 0.0
#define RANDOMIZE_FACTOR_CHARS 0.2
char messages[][150] = {
  "(_~,,~_) **The ghost**: You know what happened in the year 1989? .. The terminal hanged, and with it, so did I!",
  "(_^..^_) **The ghost**: Seriously! Are you even typing?",
  "(_O..O_) **The ghost**: Your history is haunted!"
};
 
char intro[][150] = {
" ________.__                    __    .__           __  .__                    .__           .__  .__   ",
" /  _____/|  |__   ____  _______/  |_  |__| ____   _/  |_|  |__   ____     _____|  |__   ____ |  | |  |  ",
"/   \\  ___|  |  \\ /  _ \\/  ___/\\   __\\ |  |/    \\  \\   __\\  |  \\_/ __ \\   /  ___/  |  \\_/ __ \\|  | |  |  ",
"\\    \\_\\  \\   Y  (  <_> )___ \\  |  |   |  |   |  \\  |  | |   Y  \\  ___/   \\___ \\|   Y  \\  ___/|  |_|  |__",
" \\______  /___|  /\\____/____  > |__|   |__|___|  /  |__| |___|  /\\___  > /____  >___|  /\\___  >____/____/",
"        \\/     \\/           \\/                 \\/             \\/     \\/       \\/     \\/     \\/        ",  
"\n A shell by: @sakshi"
};
 
int (*builtin_func[]) (char **) = {
  lsh_cd,
  lsh_help,
  lsh_exit
};
 
 
double get_random_value() {
  return (rand()%100)/100.0;
}
 
int is_background_process = 0;
 
/*
  Builtin function implementations.
*/
 
/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}
 
/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");
 
  for (i = 0; i < BUILTIN_COMMAND_COUNT; i++) {
    printf("  %s\n", builtin_str[i]);
  }
 
  printf("Use the man command for information on other programs.\n");
  return 1;
}
 
/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}
 
/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;
 
  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    if (is_background_process) {
      return 1;
    }
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
 
  return 1;
}
 
/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;
 
  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }
 
  if (get_random_value() < RANDOMIZE_FACTOR_MESSAGE) {
    int message_index = rand()%3;
    printf("%s\n", messages[message_index]);
    return 1;
  }
 
  for (i = 0; i < BUILTIN_COMMAND_COUNT; i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
 
  return lsh_launch(args);
}
 
#define LSH_RL_BUFSIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line()
{
  char *buffer = malloc(sizeof(char) * LSH_RL_BUFSIZE);
  fgets(buffer, LSH_RL_BUFSIZE, stdin);
 
  return buffer;
}
 
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  // line -> [  cat  main.c  ]
  // current_token -> [cat\0]
  // tokens = { "cat", "main.c" }
  is_background_process = 0;
  char **tokens = malloc(sizeof(char*) * LSH_TOK_BUFSIZE);
  memset(tokens, 0, LSH_TOK_BUFSIZE);
  char *current_token = malloc(sizeof(char) * 256);
  int pos = 0, token_pos = 0, token_count = 0;
  do {
    if (line[pos] != ' ' && line[pos] != 10) {
      current_token[token_pos] = line[pos];
      token_pos++;
    } else if (token_pos > 0) {
      current_token[token_pos] = '\0';
      if (line[pos] == 10 && token_pos == 1 && current_token[0] == '&') {
        is_background_process = 1;
      } else {
        tokens[token_count] = current_token;
        token_count++;
      }
      token_pos = 0;
      current_token = malloc(sizeof(char) * 256);
    }
    pos++;
  } while(line[pos] != '\0');
 
  free(current_token);
  return tokens;
}
 
/**
   @brief Loop getting input and executing it.
 */
void lsh_loop()
{
  char *line;
  char **args;
  int status;
 
  do {
    printf("haunted@shell$ ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);
 
    int i = 0;
    while (args[i] != NULL) {
      free(args[i]);
      i++;
    }
    free(line);
    free(args);
  } while (status);
}
 
/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  int i;
  for (i = 0; i < 7; i++) {
    printf("%s\n", intro[i]);
  }
  time_t t;
  srand((unsigned) time(&t));
  // Load config files, if any.
 
  // Run command loop.
  lsh_loop();
 
  // Perform any shutdown/cleanup.
 
  return EXIT_SUCCESS;
}
