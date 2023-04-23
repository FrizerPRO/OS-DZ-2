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

char agent_counter_shar_name[] = "/agent-counter"; // имя разделяемой памяти
int agent_counter_id; // дескриптор объекта памяти

const char *shar_object = "/posix-shar-object";
int buf_id;      // дескриптор объекта памяти
library *buffer; // указатель на разделямую память, хранящую буфер

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
    if (shm_unlink(shar_object) == -1)
    {
        perror("shm_unlink");
        // exit(-1);
    }
    if (shm_unlink(agent_counter_shar_name) == -1)
    {
        perror("shm_unlink");
        // exit(-1);
    }

    close(buf_id);
    close(agent_counter_id);
    printf("Reader: bye!!!\n");
    exit(10);
}

int main()
{
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);

    // Создание семафора
    if ((p_sem = sem_open(sem_name, O_CREAT, 0666, 1)) == 0)
    {
        perror("sem_open: Can not create posix semaphore");
        exit(-1);
    };

    

    // создаю разделяемую память для хранения библиотеки
    if ((buf_id = shm_open(shar_object, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1)
    {
        printf("Object is already open\n");
        perror("shm_open");
        return 1;
    }
    else
    {
        printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
    }

    

    if (ftruncate(buf_id, sizeof(library)) == -1)
    {
        printf("Memory sizing error\n");
        perror("ftruncate");
        return 1;
    }
    else
    {
        printf("Memory size set and = %d\n", sizeof(library));
    }



    srand(time(NULL)); // инициализация генератора случайных чисел
    while (1)
    {
        sleep(1);
        // Обнуленный семафор ожидает, когда его поднимут, чтобы вычесть 1
        if (sem_wait(p_sem) == -1)
        {
            perror("sem_wait: Incorrect wait of posix semaphore");
            exit(-1);
        };
        buffer = mmap(0, sizeof(library), PROT_WRITE | PROT_READ, MAP_SHARED, buf_id, 0);
        if (buffer == (library *)-1)
        {
            perror("process: mmap");
            exit(-1);
        }
        // добавляю книгу в конец каталога
        printf("Текущий каталог:\n");
        printLibrary(buffer);
        if (sem_post(p_sem) == -1)
        {
            perror("sem_post: Incorrect post of posix semaphore");
            exit(-1);
        };
    }

    // Семафор дождался второго процесса

    if (sem_close(p_sem) == -1)
    {
        perror("sem_close: Incorrect close of posix semaphore");
        exit(-1);
    };

    if (sem_unlink(sem_name) == -1)
    {
        perror("sem_unlink: Incorrect unlink of posix semaphore");
        exit(-1);
    };

    return 0;
}
