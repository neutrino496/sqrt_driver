#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#define BUFFER_SIZE 100

void *write_thread(int *arg) {
    int fd = *arg;
	int numbers[8];
	int num_of_elements;
    srand(time(NULL));
int i;
    while (1) {
        sleep(rand() % 5 + 1); // Sleep for 1-5 seconds
         
		num_of_elements= 4 + (rand() % 5);
for (i = 0; i < num_of_elements; i++)
{
numbers[i] = (rand()%32768)+1; // Generate a random number
        dprintf(fd, "%u ",(unsigned int) (numbers[i]*numbers[i]));
		}
		
    }

    return NULL;
}

void *read_thread(int *arg) {
    int fd = *(arg);
    char buffer[BUFFER_SIZE];
	long int bytes_read;
	unsigned int number,parsed_sqrt,br;
    while (1) {
        sleep(rand() % 3 + 3); // Sleep for 3-5 seconds
        lseek(fd, 0, SEEK_SET); // Move the file offset to the beginning
		bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Read from driver: %s\n", buffer);
        }
		int parsed = sscanf(buffer, "broj %u - %u:%u", &br, &number, &parsed_sqrt);
            if (parsed == 3) {
                if (number == (parsed_sqrt*parsed_sqrt)) {
                    printf("Calculated square root is correct!\n");
                } else {
                    printf("Calculated square root is incorrect!\n");
                }
            }
    }

    return NULL;
}

int main() {
    int fd = open("/dev/sqrt", O_RDWR);

    if (fd == -1) {
        printf("Failed to open the driver");
        return EXIT_FAILURE;
    }

    pthread_t write_id, read_id;
    
    // Create write thread
    if (pthread_create(&write_id, NULL, write_thread, &fd) != 0) {
        printf("Failed to create write thread");
        return EXIT_FAILURE;
    }

    // Create read thread
    if (pthread_create(&read_id, NULL, read_thread, &fd) != 0) {
        printf("Failed to create read thread");
        return EXIT_FAILURE;
    }

    // Wait for threads to finish
    pthread_join(write_id, NULL);
    pthread_join(read_id, NULL);

    close(fd);
    return EXIT_SUCCESS;
}