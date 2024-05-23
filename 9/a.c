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

//SOME STUFF
int fd;
const char name[] = "pipe.fifo";
struct sembuf opbuf = {.sem_num = 0, .sem_flg = 0};
int last_proc = -1;
char* stroka;

//SEMAPHORES DECLARATION
const char* sem_object_proc = "/proc-counter";
const char* sem_object_send = "/sem_object_send";
const char* sem_object_part = "/sem_object_part";
const char* sem_object_last_proc = "/last_proc";
key_t sem_proc_key = 777, sem_send_key = 776, sem_part_key = 775, sem_last_proc_key = 774, sem_b_pid_key = 700;
int sem_proc, sem_send, sem_part, sem_last_proc, sem_b_pid;

void exit_func(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    opbuf.sem_op  = -1;
    semop(sem_proc, &opbuf, 1);
    int proc_count = semctl (sem_proc, 0, GETVAL, 0);
    if (proc_count == 0) {
      kill(semctl(sem_b_pid, 0, GETVAL, 0), SIGUSR1);
      semctl(sem_proc, 0, IPC_RMID, 0);
      semctl(sem_send, 0, IPC_RMID, 0);
      semctl(sem_part, 0, IPC_RMID, 0);
      printf("Proc %d cleaning files!!!\n", getpid());
    }
    if (signal == SIGINT) {
      //FIRST LAUNCHED PROC
      if (last_proc < 0) {
        last_proc = semctl(sem_last_proc, 0, GETVAL, 0);
      }
      if (last_proc > 0 && last_proc != getpid()) {
        kill(last_proc, SIGINT);
        printf("I KILLED PROC %d\n", last_proc);
      }
    }
    close(fd);
    printf("Proc %d died\n", getpid());
    exit(0);
  }
}

int main(int argn, char* argv[]) {
  if (argn != 2) {
    printf("Incorrect args!!!\n");
    exit(-1);
  }
  srand(time(0));
  signal(SIGINT, exit_func);
  signal(SIGTERM, exit_func);
  
  if ((fd = open(name, O_CREAT | O_EXCL | O_WRONLY, 0666)) < 0) {
    fd = open(name, O_WRONLY, 0666);
  }
  else {
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
  
  //SEMAPHORES INIT
  if ((sem_proc = semget(sem_proc_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_proc = semget(sem_proc_key, 1, 0);
  }
  else {
    semctl(sem_proc, 0, SETVAL, 0);
  }

  if ((sem_send = semget(sem_send_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_send = semget(sem_send_key, 1, 0);
  }
  else {
    semctl(sem_send, 0, SETVAL, 1);
  }

  if ((sem_part = semget(sem_part_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_part = semget(sem_part_key, 1, 0);
  }
  else {
    semctl(sem_part, 0, SETVAL, 0);
  }

  if ((sem_last_proc = semget(sem_last_proc_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_last_proc = semget(sem_last_proc_key, 1, 0);
    last_proc = semctl(sem_last_proc, 0, GETVAL, 0);
    if (last_proc == 0)
      last_proc = -1;
  }
  printf("Proc to remove = %d\n", last_proc);
  semctl(sem_last_proc, 0, SETVAL, getpid());

  sem_b_pid = semget(sem_b_pid_key, 1, 0);

  //MAIN LOGIC
  opbuf.sem_op  = 1;
  semop(sem_proc, &opbuf, 1);
  int sem_value = semctl(sem_part, 0, GETVAL, 0);
  while (sem_value < number_of_parts) {
    opbuf.sem_op = 1;
    semop(sem_part, &opbuf, 1);
    printf("Proc %d working at part %d\n", getpid(), sem_value);
    sleep(rand() % 5 + 1);
    char first = stroka[sem_value * 2];
    char second = stroka[sem_value * 2 + 1];
    mytype t = {.pid = getpid(), .c = {first, second}, .part = sem_value};
    opbuf.sem_op = -1;
    semop(sem_send, &opbuf, 1);
    printf("\nProc %d finished part %d. \nSENDING...\n", getpid(), sem_value);
    write(fd, &t, sizeof(mytype));
    opbuf.sem_op = 1;
    semop(sem_send, &opbuf, 1);
    sem_value = semctl(sem_part, 0, GETVAL, 0);
  }
  exit_func(SIGTERM);
  return 0;
}