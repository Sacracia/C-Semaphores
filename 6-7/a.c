#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>

//SEMAPHORES DECLARATION
const char* shar_object_sem_part = "/shar_object_sem_part";
const char* shar_object_sem_print = "/shar_object_sem_print";
int shar_sem_part_id, shar_sem_print_id;
sem_t *sem_part, *sem_print;

//MEMORY DECLARATION
const char* shar_object_string = "shar_object_string";
int shar_string_id;
int* string;

//INPUT DATA
char stroka[6000];

int number_of_parts;
int main_pid;

//PRINT FUNC
void print_string() {
  int len = strlen(stroka);
  for (int i = 0; i < len; ++i) {
    if (*(string + i) == 0)
      printf("?? ");
    else
      printf("%#02x ", *(string + i));
  }
  printf("\n");
}

//EXIT FUNC
void exit_func(int signal) {
  if (signal == SIGINT) {
    if (getpid() == main_pid) {
      printf("\nRESULT:\n");
      print_string();
      printf("\n");
      close(shar_sem_part_id);
      close(shar_sem_print_id);
      close(shar_string_id);
      shm_unlink(shar_object_sem_part);
      shm_unlink(shar_object_sem_print);
      shm_unlink(shar_object_string);
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
  signal(SIGINT, exit_func);
  FILE* f = fopen(argv[1], "r");
  if (f == NULL) {
    printf("File not found!\n");
    exit(-1);
  }
  fgets(stroka, 6000, f);
  number_of_parts = (strlen(stroka) + 1) / 2;
  main_pid = getpid();
  
  //SEM_PART
  shar_sem_part_id = shm_open(shar_object_sem_part, O_CREAT | O_EXCL | O_RDWR, 0666);
  ftruncate(shar_sem_part_id, sizeof(sem_t));
  sem_part = mmap(0, sizeof(sem_t), PROT_WRITE | PROT_READ, MAP_SHARED, shar_sem_part_id, 0);
  sem_init(sem_part, 1, 0);
  //SEM_PRINT
  shar_sem_print_id = shm_open(shar_object_sem_print, O_CREAT | O_EXCL | O_RDWR, 0666);
  ftruncate(shar_sem_print_id, sizeof(sem_t));
  sem_print = mmap(0, sizeof(sem_t), PROT_WRITE | PROT_READ, MAP_SHARED, shar_sem_print_id, 0);
  sem_init(sem_print, 1, 1);
  //STRING MEMORY
  shar_string_id = shm_open(shar_object_string, O_CREAT | O_EXCL | O_RDWR, 0666);
  ftruncate(shar_string_id, strlen(stroka) + 1);
  string = mmap(0, strlen(stroka) + 1, PROT_WRITE | PROT_READ, MAP_SHARED, shar_string_id, 0);

  //PROC1
  if (fork() == 0) {
    int sem_value;
    sem_getvalue(sem_part, &sem_value);
    while (sem_value < number_of_parts) {
      sem_post(sem_part);
      printf("Proc1 working at part %d\n", sem_value);
      sleep(rand() % 2 + 1);
      *(string + sem_value * 2) = stroka[sem_value * 2];
      *(string + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
      sem_wait(sem_print);
      printf("\nProc1 finished part %d\n", sem_value);
      print_string();
      sem_post(sem_print);
      sem_getvalue(sem_part, &sem_value);
    }
  }
  else {
    //PROC2
    if (fork() == 0) {
      int sem_value;
      sem_getvalue(sem_part, &sem_value);
      while (sem_value < number_of_parts) {
        sem_post(sem_part);
        printf("Proc2 working at part %d\n", sem_value);
        sleep(rand() % 2 + 1);
        *(string + sem_value * 2) = stroka[sem_value * 2];
        *(string + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
        sem_wait(sem_print);
        printf("\nProc2 finished part %d\n", sem_value);
        print_string();
        sem_post(sem_print);
        sem_getvalue(sem_part, &sem_value);
      }
    }
    //PROC3
    else {
      int sem_value;
      sem_getvalue(sem_part, &sem_value);
      while (sem_value < number_of_parts) {
        sem_post(sem_part);
        printf("Proc3 working at part %d\n", sem_value);
        sleep(rand() % 2 + 1);
        *(string + sem_value * 2) = stroka[sem_value * 2];
        *(string + sem_value * 2 + 1) = stroka[sem_value * 2 + 1];
        sem_wait(sem_print);
        printf("\nProc3 finished part %d\n", sem_value);
        print_string();
        sem_post(sem_print);
        sem_getvalue(sem_part, &sem_value);
      }
      wait(0);
    }
    wait(0);
  }
  exit_func(SIGINT);
}
