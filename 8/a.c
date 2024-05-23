#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>

//INPUT
char stroka[6000];

//SEMAPHORES DECLARATION
const char* sem_object_proc = "/proc-counter";
const char* sem_object_print = "/sem_object_print";
const char* sem_object_part = "/sem_object_part";
const char* sem_object_last_proc = "/last_proc";
key_t sem_proc_key = 777, sem_print_key = 776, sem_part_key = 775, sem_last_proc_key = 774;;
int sem_proc, sem_print, sem_part, sem_last_proc;

//MEMORY DECLARATION
const char* mem_object = "/memory";
key_t mem_key = 666;
int mem_id;
int* mem;

int last_proc = -1;
struct sembuf opbuf = {.sem_num = 0, .sem_flg = 0};

//PRINT FUNC
void print_string() {
  int len = strlen(stroka);
  for (int i = 0; i < len; ++i) {
    if (*(mem + i) == 0)
      printf("?? ");
    else
      printf("%#02x ", *(mem + i));
  }
  printf("\n");
}

void exit_func(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    opbuf.sem_op  = -1;
    semop(sem_proc, &opbuf, 1);
    shmdt(mem);
    int proc_count = semctl (sem_proc, 0, GETVAL, 0);
    if (proc_count == 0) {
      semctl(sem_proc, 0, IPC_RMID, 0);
      semctl(sem_print, 0, IPC_RMID, 0);
      semctl(sem_part, 0, IPC_RMID, 0);
      semctl(sem_last_proc, 0, IPC_RMID, 0);
      shmctl (mem_id, IPC_RMID, 0);
      printf("Proc %d cleaning files!!!\n", getpid());
    }
    else if (signal == SIGINT) {
      //FIRST LAUNCHED PROC
      if (last_proc < 0) {
        last_proc = semctl(sem_last_proc, 0, GETVAL, 0);
      }
      kill(last_proc, SIGINT);
      printf("I KILLED PROC %d\n", last_proc);
    }
    printf("Proc %d died\n", getpid());
    exit(0);
  }
}

int main(int argn, char* argv[]) {
  if (argn != 2) {
    printf("Incorrect args!\n");
    exit(-1);
  }
  signal(SIGINT, exit_func);
  signal(SIGTERM, exit_func);
  
  srand(time(0));
  FILE* f = fopen(argv[1], "r");
  if (f == NULL) {
    printf("File not found!\n");
    exit(-1);
  }
  fgets(stroka, 6000, f);
  int number_of_parts = (strlen(stroka) + 1) / 2;

  //SEMAPHORES INIT
  if ((sem_proc = semget(sem_proc_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_proc = semget(sem_proc_key, 1, 0);
  }
  else {
    semctl(sem_proc, 0, SETVAL, 0);
  }

  if ((sem_print = semget(sem_print_key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    sem_print = semget(sem_print_key, 1, 0);
  }
  else {
    semctl(sem_print, 0, SETVAL, 1);
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
  }
  //printf("Proc to remove = %d\n", last_proc);
  semctl(sem_last_proc, 0, SETVAL, getpid());

  //MEMORY INIT
  if ((mem_id = shmget(mem_key, strlen(stroka) + 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
    mem_id = shmget(mem_key, strlen(stroka) + 1, 0);
  }
  mem = shmat(mem_id, NULL, 0);

  //INCREASE PROC COUNTER
  opbuf.sem_op  = 1;
  semop(sem_proc, &opbuf, 1);

  int sem_value = semctl(sem_part, 0, GETVAL, 0);
  while (sem_value < number_of_parts) {
    opbuf.sem_op = 1;
    semop(sem_part, &opbuf, 1);
    printf("Proc %d working at part %d\n", getpid(), sem_value);
    sleep(rand() % 5 + 1);
    *(mem + sem_value * 2) = stroka[sem_value * 2];
    *(mem + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
    opbuf.sem_op = -1;
    semop(sem_print, &opbuf, 1);
    printf("\nProc %d finished part %d\n", getpid(), sem_value);
    print_string();
    opbuf.sem_op = 1;
    semop(sem_print, &opbuf, 1);
    sem_value = semctl(sem_part, 0, GETVAL, 0);
  }
  
  exit_func(SIGTERM);
  return 0;
}