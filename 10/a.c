#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>

typedef struct mytype{
  int pid;
  int part;
  char c[2];
} mytype;

const char name[] = "pipe.fifo";
const char sem_object_part[] = "/sem_part";
const char sem_object_send[] = "/sem_send";
const char sem_object_proc[] = "/sem_proc";

int fd, len, last_proc, proc_fd;
char* stroka;
sem_t *sem_part, *sem_send, *sem_proc;

void exit_func(int signal) {
  if (signal == SIGINT) {
    if (last_proc < 0)
      read(proc_fd, &last_proc, 4);
    if (last_proc != getpid() && last_proc > 0) {
      kill(last_proc, SIGINT);
      printf("KILLING %d\n", last_proc);
    }
  }
  if (signal == SIGTERM || signal == SIGINT) {
    sem_wait(sem_proc);
    int proc_num;
    sem_getvalue(sem_proc, &proc_num);
    if (proc_num == 0) {
      printf("\nI am the last proc!\n");
    }
    free(stroka);
    close(fd);
    close(proc_fd);
    sem_close(sem_part);
    sem_close(sem_send);
    printf("I died\n");
    exit(0);
  }
}

int main(int argn, char* argv[]) {
  if (argn != 2) {
    printf("Incorrect number of args\n");
    exit(-1);
  }
  
  signal(SIGINT, exit_func);
  signal(SIGTERM, exit_func);
  
  srand(time(0));
  //CHECK IF B IS RUNNING
  if ((fd = open(name, O_WRONLY, 0666)) < 0) {
    printf("First, run b!!!\n");
    unlink(name);
    exit(-1);
  }
  
  //PREPARE STRING
  struct stat st;
  stat(argv[1], &st);
  int number_of_parts = (st.st_size + 1) / 2;
  stroka = malloc(st.st_size + 1);
  fgets(stroka, st.st_size + 1, fopen(argv[1], "r"));
  
  //SEMS OPEN
  sem_part = sem_open(sem_object_part, O_RDWR, 0666);
  sem_send = sem_open(sem_object_send, O_RDWR, 0666);
  sem_proc = sem_open(sem_object_proc, O_RDWR, 0666);  

  proc_fd = open("proc.fifo", O_RDWR | O_NONBLOCK, 0666);
  read(proc_fd, &last_proc, 4);
  int pid = getpid();
  write(proc_fd, &pid, 4);
  
  //LOGIC
  sem_post(sem_proc);
  sleep(1);
  int cur_part;
  sem_getvalue(sem_part, &cur_part);
  while (cur_part < number_of_parts) {
    sem_post(sem_part);
    printf("Proc %d working at part %d\n", getpid(), cur_part);
    sleep(rand() % 5 + 1);
    char first = stroka[cur_part * 2];
    char second = stroka[cur_part * 2 + 1];
    mytype t = {.pid = getpid(), .c = {first, second}, .part = cur_part};
    //SENDING
    sem_wait(sem_send);
    printf("\nProc %d finished part %d. \nSENDING...\n", getpid(), cur_part);
    write(fd, &t, sizeof(mytype));
    sem_post(sem_send);
    sem_getvalue(sem_part, &cur_part);
  }
  exit_func(SIGTERM);
  exit(0);
}