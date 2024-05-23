#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>

const char* shar_object = "/posix-shar-object";
const char* sem_name = "/posix-semaphore";
const char* print_sem_name = "/posix-print-semaphore";
int fd;
int* ptr;
sem_t *p_sem, *print_sem;
int number_of_parts;
int main_pid;

char stroka[6000];

void print_string() {
  int len = strlen(stroka);
  for (int i = 0; i < len; ++i) {
    if (*(ptr + i) == 0)
      printf("?? ");
    else
      printf("%#02x ", *(ptr + i));
  }
  printf("\n");
}

void exit_func(int sig) {
  if (sig == SIGINT) {
    if (main_pid == getpid()) {
      int len = strlen(stroka);
      printf("\nRESULT:\n");
      print_string();
      printf("\n");
      shm_unlink(shar_object);
      close(fd);
      sem_close(p_sem);
      sem_close(print_sem);
      sem_unlink(print_sem_name);
      sem_unlink(sem_name);
    }
    exit(0);
  }
}

int main(int argn, char* argv[]) {
  if (argn != 2) {
    printf("Incorrect args!\n");
    exit(-1);
  }
  srand(time(NULL));
  FILE* f = fopen(argv[1], "r");
  if (f == NULL) {
    printf("File not found!\n");
    exit(-1);
  }
  fgets(stroka, 6000, f);
  number_of_parts = (strlen(stroka) + 1) / 2;
  main_pid = getpid();
  signal(SIGINT, exit_func);
  //MEMORY
  fd = shm_open(shar_object, O_CREAT | O_EXCL | O_RDWR, 0666);
  ftruncate(fd, strlen(stroka) + 1);
  ptr = mmap(0, strlen(stroka) + 1, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
  //SEMAPHORE
  p_sem = sem_open(sem_name, O_CREAT | O_EXCL, 0666, 0);
  print_sem = sem_open(print_sem_name, O_CREAT | O_EXCL, 0666, 1);

  //PROC1
  if (fork() == 0) {
    int sem_value;
    sem_getvalue(p_sem, &sem_value);
    while (sem_value < number_of_parts) {
      sem_post(p_sem);
      printf("Proc1 working at part %d\n", sem_value);
      sleep(rand() % 2 + 1);
      *(ptr + sem_value * 2) = stroka[sem_value * 2];
      *(ptr + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
      sem_wait(print_sem);
      printf("\nProc1 finished part %d\n", sem_value);
      print_string();
      sem_post(print_sem);
      sem_getvalue(p_sem, &sem_value);
    }
  }
  else {
    //PROC2
    if (fork() == 0) {
      int sem_value;
      sem_getvalue(p_sem, &sem_value);
      while (sem_value < number_of_parts) {
        sem_post(p_sem);
        printf("Proc2 working at part %d\n", sem_value);
        sleep(rand() % 2 + 1);
        *(ptr + sem_value * 2) = stroka[sem_value * 2];
        *(ptr + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
        sem_wait(print_sem);
        printf("\nProc2 finished part %d\n", sem_value);
        print_string();
        sem_post(print_sem);
        sem_getvalue(p_sem, &sem_value);
      }
    }
    //PROC3
    else {
      int sem_value;
      sem_getvalue(p_sem, &sem_value);
      while (sem_value < number_of_parts) {
        sem_post(p_sem);
        printf("Proc3 working at part %d\n", sem_value);
        sleep(rand() % 2 + 1);
        *(ptr + sem_value * 2) = stroka[sem_value * 2];
        *(ptr + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
        sem_wait(print_sem);
        printf("\nProc3 finished part %d\n", sem_value);
        print_string();
        sem_post(print_sem);
        sem_getvalue(p_sem, &sem_value);
      }
      wait(0);
    }
    wait(0);
  }
  exit_func(SIGINT);
  exit(0);
}