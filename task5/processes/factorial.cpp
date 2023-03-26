#include <common/big_integer.hpp>

#include <thread>
#include <iostream>

#include <semaphore.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


namespace {
  using Integer = big_numbers::BigInteger;
}

int main(int argc, char** argv) {
  size_t threads = 0;

  if (argc < 2) {
    threads = std::thread::hardware_concurrency() + 1;
  } else {
    try {
      threads = std::stoul(argv[1]);
    } catch (const std::exception &ex) {
      std::cerr << "Cannot parse number of threads " << ex.what() << std::endl;
      threads = std::thread::hardware_concurrency() + 1;
    }
  }

  key_t shmkey = ftok("/dev/null", 5);
  int shmid = shmget(shmkey, sizeof (int), 0644 | IPC_CREAT);

  if (shmid < 0){
    std::cerr << "error while shmget" << std::endl;
    return 1;
  }

  int* ptr = static_cast<int*>(shmat(shmid, NULL, 0));
  *ptr = 0;
  uint32_t sem_val;
  sem_t* sem_ptr = sem_open("pSemaphore", O_CREAT | O_EXCL, 0644, sem_val);

  int next;
  std::vector<pid_t> subprocesses;

  pid_t cur_pid;
  while (std::cin >> next) {
    if (next < 0){
      std::cerr << "Cannot calculate negative factorial" << std::endl;
      continue;
    }


    cur_pid = fork();
    if (cur_pid < 0){
      std::cerr << "Error while lauching process" << std::endl;
      sem_unlink("pSemaphore");
      sem_close(sem_ptr);
      return 0;
    } else if (cur_pid == 0){
      sem_wait(sem_ptr);
      Integer res{1};
      // `next` and `stdout` will be copied 
      for (uint32_t cur = 2; cur <= next; ++cur) {
        res *= cur;
      }

      std::cout << next << "! = " << res << std::endl;
      sem_post(sem_ptr);

      return 0;
    } 
  }

  if (cur_pid > 0){
    while ((cur_pid = waitpid(-1, NULL, 0))) {
      if (errno == ECHILD) break;
    }

    shmdt(ptr);
    shmctl(shmid, IPC_RMID, 0);
    sem_unlink("pSemaphore");
    sem_close(sem_ptr);
  }

  return 0;
}

