#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "crypt.h"
#include "md5.h"
#include "util.h"
#include "ctype.h"


int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_mysleep(char **args);
int lsh_mymd5(char **args);
int lsh_myecho(char **args);


char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "mysleep",
  "mymd5",
  "myecho"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_mysleep,
  &lsh_mymd5,
  &lsh_myecho
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}



//Inicijlaizacija dodatnih funkcija

static struct md5 s;
struct crypt_ops md5_ops = {
	md5_init,
	md5_update,
	md5_sum,
	&s,
};


int lsh_myecho(char **args)
{
    int nflag = 0;
    int i = 1;

    if (args[1] && !strcmp(args[1], "-n")) {
        nflag=1;
        i=2;
        while (args[i] != NULL){
            putword(stdout, args[i]);
            
            i++;
        } 
    } else {
          while (args[i] != NULL){
             putword(stdout, args[i]);
             i++;
          } 
    }
 
    if (!nflag){
	putchar('\n');
    }

    return 1;
}

int lsh_mymd5(char **args)
{
    if (args[1]==NULL){
        puts("Nedostaje parametar!");
        return 1;
    }

    int ret = 0, (*cryptfunc)(int, char **, struct crypt_ops *, uint8_t *, size_t) = cryptmain;
	uint8_t md[MD5_DIGEST_LENGTH];
    char **arg1=&args[1];
    
    
	ret |= cryptfunc(1, arg1, &md5_ops, md, sizeof(md));
    
    return 1;

}


int lsh_mysleep(char **args)
{
    unsigned seconds;

    if (args[1]==NULL){
        puts("Nedostaje parametar!");
        return 1;
    }

	seconds = estrtonum(args[1], 0, UINT_MAX);
	while ((seconds = sleep(seconds)) > 0)
		;

	return 1;
}
    
    


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


int lsh_help(char **args)
{
  int i;
  printf("MyShell\n");
  printf("Unesite naziv programa i argumente.\n");
  printf("Slijedeći su ugrađeni:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}


int lsh_exit(char **args)
{
  return 0;
}


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
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
   
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024

char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}


void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}


int main(int argc, char **argv)
{

  lsh_loop();



  return EXIT_SUCCESS;
}

