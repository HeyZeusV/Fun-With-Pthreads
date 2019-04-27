#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define MAX_THREADS 16
#define MAX_ITERATIONS 40

// to change output color
// https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
#define ANSI_COLOR_CYAN  "\x1b[36m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

// global variable
int counter = 0;
pthread_mutex_t lock;

/**
 * use this struct as a parameter for the function
 * nanosleep. 
 * For exmaple : nanosleep(&ts, NULL);
 */
struct timespec ts = {0, 100000000};

// modified your struct
struct ThreadArgs {

    pthread_t tid;
    int id;
};

void *addValue(void *arg) {

	struct ThreadArgs* args = (struct ThreadArgs*)arg;
	int temp;

	for (int y = 0; y < MAX_ITERATIONS; y++) {

		pthread_mutex_lock(&lock);
		temp = counter;
		nanosleep(&ts, NULL);
		temp += 3;
		counter = temp;
		printf(ANSI_COLOR_GREEN "Current Value written to Global Variables by ADDER      thread id: %-4d is %d\n", args->id, temp);
		pthread_mutex_unlock(&lock);
	}
}

void *subValue(void *arg) {

	struct ThreadArgs* args = (struct ThreadArgs*)arg;
	int temp;

	for (int y = 0; y < MAX_ITERATIONS; y++) {

		pthread_mutex_lock(&lock);
		temp = counter;
		nanosleep(&ts, NULL);
		temp -= 3;
		counter = temp;
		printf(ANSI_COLOR_CYAN "Current Value written to Global Variables by SUBTRACTOR thread id: %-4d is %d\n", args->id, temp);
		pthread_mutex_unlock(&lock);
	}
}

int main(int argc, char** argv) {

	struct ThreadArgs threads[MAX_THREADS];
	int ret_val;

	if (pthread_mutex_init(&lock, NULL) != 0) {

		perror("Mutix init has failed...");
		return -2;
	}

	// creates 16 threads
	for (int i = 0; i < MAX_THREADS; i++) {

		// evens
		if ((i % 2) == 0) {

			threads[i].id = i + 1;
			ret_val = pthread_create(&threads[i].tid, NULL, addValue, (void *)&threads[i]);
			if (ret_val < 0) {

            	perror("Error creating thread..");
            	return -2;
        	}	
        // odds		
		} else {

			threads[i].id = i + 1;
			ret_val = pthread_create(&threads[i].tid, NULL, subValue, (void *)&threads[i]);
			if (ret_val < 0) {

            	perror("Error creating thread..");
            	return -2;
        	}	
		}
	}

	for (int i = 0; i < MAX_THREADS; i++) {

        ret_val = pthread_join(threads[i].tid, NULL);
        printf(ANSI_COLOR_RESET"Total threads joined: %i\n", i + 1);
        if (ret_val) {
            perror("Error joining thread: ");
            return -3;
        }
    }
	// prints the final count
	printf(ANSI_COLOR_RESET"Final Value of Shared Variable: %i\n", counter);

    return 0;
}