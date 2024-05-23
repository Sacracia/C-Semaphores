#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct mytype{
  int pid;
  int part;
  char c[2];
} mytype;

const char name[] = "pipe.fifo";
int len;
char* mem;
int fd;
int toExit = 0;

//SEM
const key_t sem_last_proc_key = 774;
const key_t sem_b_pid_key = 700;
int sem_last_proc, sem_b_pid;

//PRINT FUNC
void print_string() {
  for (int i = 0; i < len; ++i) {
    if (*(mem + i) == 0)
      printf("?? ");
    else
      printf("%#02x ", *(mem + i));
  }
  printf("\n");
}

void exit_func(int signal) {
  if (signal == SIGUSR1) {
    toExit = 1;
  }
  if (signal == SIGINT || signal == SIGTERM) {
    if (signal == SIGINT) {
      int last = semctl(sem_last_proc, 0, GETVAL, 0);
      if (last > 0) {
        kill(semctl(sem_last_proc, 0, GETVAL, 0), SIGINT);
        printf("killed %d\n", semctl(sem_last_proc, 0, GETVAL, 0));
      }
    }
    semctl(sem_last_proc, 0, IPC_RMID, 0);
    semctl(sem_b_pid, 0, IPC_RMID, 0);
    free(mem);
    close(fd);
    unlink(name);
    printf("B (proc %d) died\n", getpid());
    exit(0);
  }
}

int main(int argn, char* argv[]) {
  if (argn != 2) {
    printf("Incorrect args!!!\n");
    exit(-1);
  }
  
  signal(SIGINT, exit_func);
  signal(SIGTERM, exit_func);
  signal(SIGUSR1, exit_func);
  
  struct stat st;
  if (stat(argv[1], &st) < 0) {
    printf("File not found!!!\n");
    exit(-1);
  }
  len = st.st_size;
  mem = malloc(len + 1);
  mknod(name, S_IFIFO | 0666, 0);
  fd = open(name, O_RDONLY | O_NONBLOCK);

  //SEM INITS
  if ((sem_last_proc = semget(sem_last_proc_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_last_proc = semget(sem_last_proc_key, 1, 0);
  }
  semctl(sem_last_proc, 0, SETVAL, -1);
  
  sem_b_pid = semget(sem_b_pid_key, 1, 0666 | IPC_CREAT | IPC_EXCL);
  semctl(sem_b_pid, 0, SETVAL, getpid());
    
  
  mytype t;
  while (1) {
    if (read(fd, &t, sizeof(mytype)) > 0) {
      *(mem + t.part * 2) = t.c[0];
      *(mem + t.part * 2 + 1) = t.c[1];
      printf("\nAfter proc %d part %d:\n", t.pid, t.part);
      print_string();
    }
    else if (toExit) {
      exit_func(SIGTERM);
    }
    sleep(1);
  }
  exit_func(SIGTERM);
}