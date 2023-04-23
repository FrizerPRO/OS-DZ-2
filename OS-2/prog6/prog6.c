// Общий модуль, осуществляющий одинаковые административные функции
// как для писателя, так и для читателя.
// Заголовочный файл, содержащий общие данные для писателей и читателей
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include <semaphore.h>

#define BUF_SIZE 10     // Размер буфера ячеек
#define KEY 1234 // Ключ для доступа к ресурсам


struct book {
    int uniq_id;
    int row_id;
    int closet_id;
    int book_id;
};
typedef struct {
    struct book books[BUF_SIZE];
    int num_books; // current number of books in the library
} library;

const char* shar_object = "/posix-shar-object";
int buf_id;        // дескриптор объекта памяти
library *buffer;    // указатель на разделямую память, хранящую буфер


void insertionSort(library *lib) {
    struct book *arr = lib->books;
    int n = lib->num_books;
    int i, j;
    struct book key;
    
    for (i = 1; i < n; i++) {
        key = arr[i];
        j = i - 1;
        
        while (j >= 0 && arr[j].uniq_id > key.uniq_id) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}
void printLibrary(library *lib) {

    struct book *arr = lib->books;
    int n = lib->num_books;
    int i;

    for (i = 0; i < n; i++) {
        printf("uniq_id = %d\t row_id = %d\t closet_id = %d\t book_id = %d\n", arr[i].uniq_id,arr[i].row_id,arr[i].closet_id,arr[i].book_id);
    }
}
    int sem_id;
void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }

  // Закрывает свой семафор
  if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(1);
    }
    if (shmctl(buf_id, IPC_RMID, NULL) == -1) {
         perror("shmctl");
         exit(1);
    }

  printf("Reader: bye!!!\n");
  exit (10);
}
int main() {   
    signal(SIGINT,sigfunc);
    signal(SIGTERM,sigfunc);

    struct sembuf acquire = {0, -1, SEM_UNDO}; // операция захвата семафора
    struct sembuf release = {0, 1, SEM_UNDO}; // операция освобождения семафора
    

      // Создаем семафор
if ((sem_id = semget(KEY, 1, IPC_CREAT | 0666)) == -1) {
    perror("Ошибка при создании семафора");
    exit(1);
  }

  // Инициализируем семафор значением 1
  if (semctl(sem_id, 0, SETVAL, 1) == -1) {
    perror("Ошибка при инициализации семафора");
    exit(1);
  }

    //УБРАТЬ ЭТО
//        if(shm_unlink(shar_object) == -1) {
//          perror("shm_unlink");
//          exit(-1);
//        }


    //создаю разделяемую память
      if ((buf_id = shmget(KEY, sizeof(library), IPC_CREAT | 0666)) == -1) {
    perror("Ошибка при создании разделяемой памяти");
    exit(1);
  } else {
        printf("Object is open: id = 0x%x\n",buf_id);
    }
      
    
    //close(buf_id);
    
    srand(time(NULL)); // инициализация генератора случайных чисел
    
    library input_books;
    
    for (int i = 0; i < BUF_SIZE; i++) {
        input_books.books[i].uniq_id = i; // генерация уникального идентификатора от 1 до 1000
        input_books.books[i].row_id = rand() % 10 + 1; // генерация номера ряда от 1 до 10
        input_books.books[i].closet_id = rand() % 5 + 1; // генерация номера шкафа от 1 до 5
        input_books.books[i].book_id = rand() % 20 + 1; // генерация номера книги от 1 до 20
    }
    
    
    for(int i = 0; i < BUF_SIZE; i++)
    {
        pid_t pid = fork(); // создаем новый процесс
        
        if (pid == -1) // обработчик ошибки
        {
            printf("Ошибка при создании дочернего процесса\n");
            exit(1);
        }
        else if (pid == 0) // код для дочернего процесса
        {
            
            
            if ((buffer = shmat(buf_id, 0, 0)) == (library *) -1) {
    		perror("Ошибка при присоединении к разделяемой памяти");
    		exit(1);
  		}
           
            
            // Обнуленный семафор ожидает, когда его поднимут, чтобы вычесть 1
                if (semop(sem_id, &acquire, 1) == -1) {
      			perror("Ошибка при захвате семафора");
      			exit(1);
    			}

            
            //добавляю книгу в конец каталога
            buffer->num_books += 1;
            buffer->books[i] = input_books.books[i];
            insertionSort(buffer);
            printf("Это дочерний процесс %d, его идентификатор %d\n", i+1, getpid());
            printf("Текущий каталог:\n");
            printLibrary(buffer);
            if (semop(sem_id, &release, 1) == -1) {
      		perror("Ошибка при освобождении семафора");
      		exit(1);
    		}
    	    if (shmdt(buffer) == -1) {
        	perror("shmdt");
        	exit(1);
    		}
            
            exit(0);
        }
    }
    while(wait(NULL)>0);
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(1);
    }
    if (shmctl(buf_id, IPC_RMID, NULL) == -1) {
         perror("shmctl");
         exit(1);
    }
    return 0;
}
