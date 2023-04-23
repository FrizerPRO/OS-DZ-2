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

#define BUF_SIZE 10 // Размер буфера ячеек

struct book
{
    int uniq_id;
    int row_id;
    int closet_id;
    int book_id;
};
typedef struct
{
    struct book books[BUF_SIZE];
    int num_books; // current number of books in the library
} library;

const char *shar_object = "/posix-shar-object";
int buf_id;      // дескриптор объекта памяти
library *buffer; // указатель на разделямую память, хранящую буфер

void insertionSort(library *lib)
{
    struct book *arr = lib->books;
    int n = lib->num_books;
    int i, j;
    struct book key;

    for (i = 1; i < n; i++)
    {
        key = arr[i];
        j = i - 1;

        while (j >= 0 && arr[j].uniq_id > key.uniq_id)
        {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}
void printLibrary(library *lib)
{
    struct book *arr = lib->books;
    int n = lib->num_books;
    int i;

    for (i = 0; i < n; i++)
    {
        printf("uniq_id = %d\t row_id = %d\t closet_id = %d\t book_id = %d\n", arr[i].uniq_id, arr[i].row_id, arr[i].closet_id, arr[i].book_id);
    }
}

char sem_name[] = "/posix-semaphore"; // имя семафора
sem_t *p_sem;                         // адрес семафора
void sigfunc(int sig)
{
    if (sig != SIGINT && sig != SIGTERM)
    {
        return;
    }

    // Закрывает свой семафор
    if (sem_close(p_sem) == -1)
    {
        perror("sem_close: Incorrect close of reader semaphore");
        exit(-1);
    };
    // Удаляет свой семафор
    if (sem_unlink(sem_name) == -1)
    {
        perror("sem_unlink: Incorrect unlink of reader semaphore");
        // exit(-1);
    };
    printf("Reader: bye!!!\n");
    exit(10);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);
    if (argc != 2)
    {
        printf("Unique index in args needed!");
        return 1;
    }
    // Создание семафора
    if ((p_sem = sem_open(sem_name, 0)) == 0)
    {
        perror("sem_open: Can not create posix semaphore");
        exit(-1);
    };

    // УБРАТЬ ЭТО
    

    // создаю разделяемую память
    if ((buf_id = shm_open(shar_object, O_RDWR, 0666)) == -1)
    {
        printf("Object is already open\n");
        perror("shm_open");
        return 1;
    }
    else
    {
        printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
    }

    // close(buf_id);

    srand(time(NULL)); // инициализация генератора случайных чисел
    struct book book_tmp;
    book_tmp.uniq_id = atoi(argv[1]);    // генерация уникального идентификатора от 1 до 1000
    book_tmp.row_id = rand() % 10 + 1;   // генерация номера ряда от 1 до 10
    book_tmp.closet_id = rand() % 5 + 1; // генерация номера шкафа от 1 до 5
    book_tmp.book_id = rand() % 20 + 1;  // генерация номера книги от 1 до 20

    buffer = mmap(0, sizeof(library), PROT_WRITE | PROT_READ, MAP_SHARED, buf_id, 0);

    if (buffer == (library *)-1)
    {
        perror("process: mmap");
        exit(-1);
    }

    // Обнуленный семафор ожидает, когда его поднимут, чтобы вычесть 1
    if (sem_wait(p_sem) == -1)
    {
        perror("sem_wait: Incorrect wait of posix semaphore");
        exit(-1);
    };

    // добавляю книгу в конец каталога
    buffer->num_books += 1;
    buffer->books[buffer->num_books - 1] = book_tmp;
    printf("%d", buffer->num_books);
    insertionSort(buffer);
    if (sem_post(p_sem) == -1)
    {
        perror("sem_post: Incorrect post of posix semaphore");
        exit(-1);
    };
    return 0;
}
