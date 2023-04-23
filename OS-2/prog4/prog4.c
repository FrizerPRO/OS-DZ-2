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


#include <semaphore.h>

#define BUF_SIZE 10     // Размер буфера ячеек

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

char sem_name[] = "/posix-semaphore"; // имя семафора
sem_t *p_sem;   // адрес семафора
void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }

  // Закрывает свой семафор
  if(sem_close(p_sem) == -1) {
    perror("sem_close: Incorrect close of reader semaphore");
    exit(-1);
  };
  // Удаляет свой семафор
  if(sem_unlink(sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of reader semaphore");
    // exit(-1);
  };
  printf("Reader: bye!!!\n");
  exit (10);
}

int main() {
  signal(SIGINT,sigfunc);
  signal(SIGTERM,sigfunc);

    
    // Создание семафора
    if((p_sem = sem_open(sem_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create posix semaphore");
        exit(-1);
    };
    
    //УБРАТЬ ЭТО
        if(shm_unlink(shar_object) == -1) {
          perror("shm_unlink");
          // exit(-1);
        }


    //создаю разделяемую память
    if ((buf_id = shm_open(shar_object, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1) {
        
        printf("Object is already open\n");
        perror("shm_open");
        return 1;
    } else {
        printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
    }
    
    
    if (ftruncate(buf_id, sizeof(library)) == -1) {
        printf("Memory sizing error\n");
        perror("ftruncate");
        return 1;
    } else {
        printf("Memory size set and = %d\n", sizeof(library));
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
            
            
            buffer = mmap(0, sizeof (library), PROT_WRITE|PROT_READ, MAP_SHARED, buf_id, 0);
            if (buffer == (library*)-1 ) {
                perror("process: mmap");
                exit(-1);
            }
            
            // Обнуленный семафор ожидает, когда его поднимут, чтобы вычесть 1
            if(sem_wait(p_sem) == -1) {
                perror("sem_wait: Incorrect wait of posix semaphore");
                exit(-1);
            };
            
            //добавляю книгу в конец каталога
            buffer->num_books += 1;
            buffer->books[i] = input_books.books[i];
            insertionSort(buffer);
            printf("Это дочерний процесс %d, его идентификатор %d\n", i+1, getpid());
            printf("Текущий каталог:\n");
            printLibrary(buffer);
            if(sem_post(p_sem) == -1) {
                perror("sem_post: Incorrect post of posix semaphore");
                exit(-1);
            };
            close(buf_id);
            
            exit(0);
        }
        else // код для родительского процесса
        {
            // ждем завершения дочернего процесса
            int status;
            waitpid(pid, &status, 0);
            
        }
    }
    
    
    // Семафор дождался второго процесса
    
    
    if(sem_close(p_sem) == -1) {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };
    
    if(sem_unlink(sem_name) == -1) {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };
    
    return 0;
}
