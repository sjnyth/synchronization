#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define SHARED_MEM_SIZE sizeof(int)

void ParentProcess(sem_t *sem, int *BankAccount);
void ChildProcess(sem_t *sem, int *BankAccount);
void MomProcess(sem_t *sem, int *BankAccount);

int main(int argc, char *argv[]) {
    int ShmID;
    int *BankAccount;
    sem_t *sem;
    pid_t pid, mom_pid;
    int status;

    // Create shared memory for the BankAccount
    ShmID = shmget(IPC_PRIVATE, SHARED_MEM_SIZE, IPC_CREAT | 0666);
    if (ShmID < 0) {
        perror("*** shmget error (server) ***");
        exit(1);
    }
    printf("Server has received a shared memory segment for BankAccount...\n");

    BankAccount = (int *)shmat(ShmID, NULL, 0);
    if (BankAccount == (int *)-1) {
        perror("*** shmat error (server) ***");
        exit(1);
    }
    printf("Server has attached the shared memory...\n");

    *BankAccount = 100; // Initialize BankAccount balance
    printf("BankAccount initialized to $%d...\n", *BankAccount);
    
    // Create a named semaphore
    sem = sem_open("/bank_semaphore", O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("*** sem_open error ***");
        exit(1);
    }

    printf("Server is about to fork child processes...\n");
    
    // Fork the "Lovable Mom" process
    mom_pid = fork();
    if (mom_pid < 0) {
        perror("*** fork error (server) ***");
        exit(1);
    } else if (mom_pid == 0) {
        MomProcess(sem, BankAccount);
        exit(0);
    }

    // Fork the "Poor Student" process
    pid = fork();
    if (pid < 0) {
        perror("*** fork error (server) ***");
        exit(1);
    } else if (pid == 0) {
        ChildProcess(sem, BankAccount);
        exit(0);
    }

    // Parent process
    ParentProcess(sem, BankAccount);

    // Wait for child processes
    waitpid(mom_pid, &status, 0);
    waitpid(pid, &status, 0);

    // Cleanup
    sem_close(sem);
    sem_unlink("/bank_semaphore");
    shmdt((void *)BankAccount);
    shmctl(ShmID, IPC_RMID, NULL);
    printf("Server has removed its shared memory and semaphore...\n");
    printf("Server exits...\n");
    exit(0);
}

void ParentProcess(sem_t *sem, int *BankAccount) {
    srand(time(0) + getpid());
    while (1) {
        sleep(rand() % 5);
        printf("Dear Old Dad: Attempting to Check Balance\n");

        sem_wait(sem);
        int localBalance = *BankAccount;

        if (rand() % 2 == 0) { // Random number is even
            if (localBalance < 100) {
                int amount = rand() % 101;
                *BankAccount += amount;
                printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", amount, *BankAccount);
            } else {
                printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(sem);
    }
}

void MomProcess(sem_t *sem, int *BankAccount) {
    srand(time(0) + getpid());
    while (1) {
        sleep(rand() % 10);
        printf("Lovable Mom: Attempting to Check Balance\n");

        sem_wait(sem);
        int localBalance = *BankAccount;

        if (localBalance <= 100) {
            int amount = rand() % 126; // Random amount between 0-125
            *BankAccount += amount;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, *BankAccount);
        } else {
            printf("Lovable Mom: Thinks balance is sufficient ($%d)\n", localBalance);
        }
        sem_post(sem);
    }
}

void ChildProcess(sem_t *sem, int *BankAccount) {
    srand(time(0) + getpid());
    while (1) {
        sleep(rand() % 5);
        printf("Poor Student: Attempting to Check Balance\n");

        sem_wait(sem);
        int localBalance = *BankAccount;

        if (rand() % 2 == 0) { // Random number is even
            int need = rand() % 51; // Random need between 0-50
            printf("Poor Student needs $%d\n", need);
            if (need <= localBalance) {
                *BankAccount -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, *BankAccount);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(sem);
    }
}