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

#include <sys/types.h>
#include <sys/stat.h>

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

char *fifo_name = "/tmp/myfifo";

void sigfunc(int sig)
{
    if (sig != SIGINT && sig != SIGTERM)
    {
        return;
    }
    if (unlink(fifo_name) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
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

    close(buf_id);
    close(agent_counter_id);
    printf("Reader: bye!!!\n");
    exit(10);
}

int main()
{    
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);
if(access(fifo_name, F_OK) == 0) { // check if the named pipe exists
        if (unlink(fifo_name) == -1) { // delete the named pipe
            perror("unlink");
            exit(EXIT_FAILURE);
        }
        printf("Named pipe '%s' deleted successfully\n", fifo_name);
    } else {
        printf("Named pipe '%s' does not exist\n", fifo_name);
    }
    // Создание семафора
    if ((p_sem = sem_open(sem_name, O_CREAT, 0666, 1)) == 0)
    {
        perror("sem_open: Can not create posix semaphore");
        exit(-1);
    };



    // создаю разделяемую память для хранения библиотеки
    if (mkfifo(fifo_name, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    library init_lib;

        int fd = open(fifo_name, O_WRONLY); // Open the FIFO for writing

        if (write(fd, &init_lib, sizeof(library)) == -1) { // Send the message to the parent process
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(fd); // Close the FIFO

    srand(time(NULL)); // инициализация генератора случайных чисел
    while (1)
    {
        sleep(1);
        // Обнуленный семафор ожидает, когда его поднимут, чтобы вычесть 1                int fd = open(fifo_name, O_RDONLY); // Open the FIFO for reading
        library buffer;
       int fd = open(fifo_name, O_RDONLY); // Open the FIFO for writing
        if (read(fd, &buffer, sizeof(library)) == -1) { // Receive the message from the child process
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Текущий каталог:\n");
        printLibrary(&buffer);
        close(fd); // Close the FIFO
         fd = open(fifo_name, O_WRONLY); // Open the FIFO for writing

        if (write(fd, &buffer, sizeof(library)) == -1) { // Send the message to the parent process
            perror("write");
            exit(EXIT_FAILURE);
        }
        close(fd); // Close the FIFO

        // добавляю книгу в конец каталога
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
