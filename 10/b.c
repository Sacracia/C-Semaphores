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
const char proc[] = "proc.fifo";
const char sem_object_part[] = "/sem_part";
const char sem_object_send[] = "/sem_send";
const char sem_object_proc[] = "/sem_proc";

char* mem;
int len, fd, proc_fd, toExit = 0;
sem_t *sem_send, *sem_part, *sem_proc;

void exit_func(int signal) {
  if (signal == SIGINT) {
    int last_proc;
    read(proc_fd, &last_proc, 4);
    if (last_proc > 0) {
      kill(last_proc, SIGINT);
      printf("KILLING %d\n", last_proc);
    }
  }
  if (signal == SIGTERM || signal == SIGINT) {
    free(mem);
    close(fd);
    close(proc_fd);
    sem_close(sem_part);
    sem_close(sem_send);
    sem_close(sem_proc);
    sem_unlink(sem_object_part);
    sem_unlink(sem_object_send);
    sem_unlink(sem_object_proc);
    unlink(name);
    unlink(proc);
    printf("\nCleaning files\n");
    printf("I\'m leaving this world!!!\n");
    exit(0);
  }
}

//PRINT
void print_string() {
  for (int i = 0; i < len; ++i) {
    if (*(mem + i) == 0)
      printf("?? ");
    else
      printf("%#02x ", *(mem + i));
  }
  printf("\n");
}

int main(int argn, char* argv[]) {
  signal(SIGINT, exit_func);
  signal(SIGTERM, exit_func);

  if (argn != 2) {
    printf("Incorrect number of args\n");
    exit(-1);
  }

  //OPEN PIPES
  if (mknod(name, S_IFIFO | 0666, 0) < 0) {
    printf("B aldready running!!!\n");
    exit(-1);
  }
  fd = open(name, O_RDONLY | O_NONBLOCK);
  mknod(proc, S_IFIFO | 0666, 0);
  proc_fd = open(proc, O_RDWR | O_NONBLOCK);
  int a = -1;
  write(proc_fd, &a, 4);
    
  //OPEN READ FILE
  struct stat st;
  if (stat(argv[1], &st) < 0) {
    printf("File not found!!!\n");
    exit(-1);
  }
  len = st.st_size;
  mem = malloc(len + 1);

  //CREATE SEMS
  sem_send = sem_open(sem_object_send, O_CREAT | O_EXCL | O_RDWR, 0666, 1);
  sem_part = sem_open(sem_object_part, O_CREAT | O_EXCL, 0666, 0);
  sem_proc = sem_open(sem_object_proc, O_CREAT | O_EXCL, 0666, 0);
  
  //LOGIC
  int proc_num = 0;
  while (proc_num == 0) {
    sleep(1);
    sem_getvalue(sem_proc, &proc_num);
  }
  
  mytype t;
  while (1) {
    if (read(fd, &t, sizeof(mytype)) > 0) {
      *(mem + t.part * 2) = t.c[0];
      *(mem + t.part * 2 + 1) = t.c[1];
      printf("\nAfter proc %d part %d:\n", t.pid, t.part);
      print_string();
    }
    else {
      sem_getvalue(sem_proc, &proc_num);
      if (proc_num == 0)       {
        exit_func(SIGTERM);
      }
    }
    sleep(1);
  }
  
  //EXIT
  exit_func(SIGTERM);
  exit(0);
}