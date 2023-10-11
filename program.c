/*
Name: Calvin Stephenson
ID# 620130499
Course: COMP3101 - Operating Systems
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

/*
Code overview

1)  The user shall enter a value that is to be the intial value of the account.

2)  The code then checks to ensure that the input is an actual number.

3)  The code them uses a series of read and write operations synchronous with with 
    the application of semaphores to achived the balance in the account.dat file and the 
    transaction record in the transaction.dat file.

4)  Each code uses a semaphore_wait is used to check if the semaphore id is 1 or 0. If is 0 this indicatees that a process is alread working in its critical section and hence the checking process must wait on that process to be completed. If the check is done and the semaphore id is 1 then the checking process my porceed with the execution of its critical section.

5)  Hence as a trivial example the user enters 0 for the input. The code then checks that 0 is a 
    number and converts it to an integer. Assuming child1 runs first the sem id is set to 0 to
    prevent child2 updating the account balance while child1 is still operating.(and visa versal 
    if child2 enters first). Following this child1 does a semaphore_signal and increments the 
    sem id, allowinf child2 to proceed. Both child reads and close the files storing the updated 
    accunt balance to ensure data integrity.
    
6)  Lastly, there is a debug_() funtion added with the intent of using for debugging purpoases
    by printing to screen.
*/


struct Transaction {
    pid_t processId;
    pid_t parentId;
    char transactionType[20];
    int balanceBefore;
    int balanceAfter;
};

void semaphore_wait(int sem_id, int sem_num) {
    struct sembuf wait_operation;
    wait_operation.sem_num = sem_num;
    wait_operation.sem_op = -1;
    wait_operation.sem_flg = 0;
    if (semop(sem_id, &wait_operation, 1) == -1) {
        perror("Error in semaphore wait");
        exit(EXIT_FAILURE);
    }
}

void semaphore_signal(int sem_id, int sem_num) {
    struct sembuf signal_operation;
    signal_operation.sem_num = sem_num;
    signal_operation.sem_op = 1;
    signal_operation.sem_flg = 0;
    if (semop(sem_id, &signal_operation, 1) == -1) {
        perror("Error in semaphore signal");
        exit(EXIT_FAILURE);
    }
}


void debug_(){
    FILE *transactionRead = fopen("transaction.dat", "r");
            if (transactionRead != NULL) {
                printf("Transaction Records:\n");
                char buffer[100];
                while (fgets(buffer, sizeof(buffer), transactionRead) != NULL) {
                    printf("%s", buffer);
                }
                fclose(transactionRead);
            } else {
                perror("file was not found");
            }

        
            FILE *accountFileRead = fopen("account.dat","r");
            int bankBalance;
            fscanf(accountFileRead,"%d",&bankBalance);
            fclose(accountFileRead);
            printf("All processes successful. Your remaining balance is: $%d\n",bankBalance);
}

int main() {
    
    // Initializng semaphore used to accomplish mutual exclusion
    char error_creating_semaphore_msg[60] = "Unsuccessful! There was a error in creating the semaphore";
    int sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror(error_creating_semaphore_msg);
        exit(EXIT_FAILURE);
    }
    semctl(sem_id, 0, SETVAL, 1);
    
    //1.a On starting up the program shall prompt the user to enter the balance in an account.
    // Also we check to ensure that it is a valid number.
    int balance;
    char input[50];
    printf("Enter the initial balance: ");
    if (fscanf(stdin,"%s",input) == 1){
        if(sscanf(input,"%d",&balance)!=1){
           perror("You have entered a balance that is not a number.\n");
           return 1;
        }
        
    }

    /*
    1.b The program shall then write a transaction record (see format below) to the 
    file transaction.dat and shall also store the starting balance in the file account.dat
    */
    FILE *accountFile = fopen("account.dat", "w");
    FILE *transactionFile = fopen("transaction.dat", "w");
    
    // error handling to ensure code does not break
    if (accountFile == NULL || transactionFile == NULL){ 
        perror("Files for account.dat / transaction.dat was not create successfully. try running again.\n");
        return 0;
    }
    else {

        fprintf(accountFile, "%d\n", balance); //writing initial balance to account.dat file (done)
        fclose(accountFile);
        fclose(transactionFile); // created the transaction.dat file for futher use.
    }
    
    pid_t child1 = fork();
    if (child1 == 0) {
        semaphore_wait(sem_id, 0);
        
         // critical section
        int child1_deposit = 40;
        int initial_balance;
        int new_balance;
        
        accountFile = fopen("account.dat", "r");
        fscanf(accountFile,"%d",&initial_balance);
        new_balance = initial_balance + child1_deposit;

        struct Transaction transaction1 = {getpid(), getppid(), "DEPOSIT", initial_balance,new_balance};
        
        transactionFile = fopen("transaction.dat", "a");
        fprintf(transactionFile, "%d,%d,%s,%d,%d\n", transaction1.processId, transaction1.parentId,
                transaction1.transactionType, transaction1.balanceBefore, transaction1.balanceAfter);
        fclose(transactionFile);
        fclose(accountFile);
        
        accountFile = fopen("account.dat", "w");
        fprintf(accountFile, "%d\n", new_balance);
        fclose(accountFile);

        semaphore_signal(sem_id, 0); // incrementing semaphore to prevent starvation
        exit(EXIT_SUCCESS);
    } else {
        pid_t child2 = fork();

        if (child2 == 0) {
            semaphore_wait(sem_id, 0);
            
            // critical section
            int initial_balance;
            int new_balance;
            
            accountFile = fopen("account.dat", "r");
            fscanf(accountFile,"%d",&initial_balance);
            
            new_balance = initial_balance - 20;
            fclose(accountFile);

            struct Transaction transaction2 = {getpid(), getppid(), "WITHDRAWAL", initial_balance, new_balance};
            
            transactionFile = fopen("transaction.dat", "a");
            fprintf(transactionFile, "%d,%d,%s,%d,%d\n", transaction2.processId, transaction2.parentId,
                    transaction2.transactionType, transaction2.balanceBefore, transaction2.balanceAfter);
            fclose(transactionFile);    
            
            accountFile = fopen("account.dat", "w");
            fprintf(accountFile, "%d\n", new_balance);
            fclose(accountFile);
            
            semaphore_signal(sem_id, 0); // incrementing semaphore to prevent starvation
            exit(EXIT_SUCCESS);
        } else {
            wait(NULL);
            wait(NULL);

            semctl(sem_id, 0, IPC_RMID);

            }
        
        // debug display's the content of transaction.dat (debugging purposses)
        // debug_();
    }

    return 0;
}
