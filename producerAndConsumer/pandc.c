#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/resource.h>

// to change output color
// https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
#define ANSI_COLOR_CYAN  "\x1b[36m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

// for semaphores
sem_t MUTEX, DATA, EMPTY;

// arguments
int N;
int P;
int C;
int X;
int Ptime;
int Ctime;

int buffer[50];
int producerArray[1000];
int consumerArray[1000];

// helps keep count for both arrays
int counter     = 0;
int Ccounter    = 0;
// used to keep track of buffer location
int next_in     = 0;
int next_out    = 0;
// checks if there will be overconsumption
int consumeOver;
int consumeAmount;
int totalConsumption;

// used for pthreads
struct ThreadArgs {

	pthread_t tid;
	int id;
};

/* 
 * Function to remove item.
 * Item removed is returned
 */
int dequeue_item() {
    
    // adds value to consumer array for later comparison
    consumerArray[Ccounter - 1] = buffer[next_out];
	int removed = buffer[next_out];
	// 'consumes' value
	buffer[next_out] = 0;
	// this wraps around buffer array once we exceed buffer size
	next_out = (next_out + 1) % N;
	return removed;
}

/* 
 * Function to add item.
 * Item added is returned.
 */
int enqueue_item(int item) {
   
    // adds value to producer array for later comparison
    producerArray[counter - 1] = item;
	buffer[next_in] = item;
	// this wraps around buffer array once we exceed buffer size
	next_in = (next_in + 1) % N;
	return item; 
}
 
// producer thread function
void *producer(void *arg) {

	// uses struct in order to print out nice thread ids
	struct ThreadArgs* args = (struct ThreadArgs*)arg;
	int value;

	for (int i = 0; i < X; i++) {

		// blocks if buffer is full, otherwise decrements empty counter
		sem_wait(&EMPTY);
		// blocks if any other producer/consumer is in their critical section
		sem_wait(&MUTEX);

		// critical section
		counter += 1;
		value = enqueue_item(counter);
		printf(ANSI_COLOR_CYAN "Producer %-2d has produced %d\n", args->id, value);
 
 		// end critical section
		sem_post(&MUTEX);
		// increments data counter
		sem_post(&DATA);

		sleep(Ptime);
	}
}

// consumer thread function
void *consumer(void *arg) {

	// uses struct in order to print out nice thread ids
	struct ThreadArgs* args = (struct ThreadArgs*) arg;
	int value;
	for (int i = 0; i < consumeAmount; i++) {
	
		// blocks if buffer is empty, otherwise decrements data counter
		sem_wait(&DATA);
		// blocks if any other producer/consumer is in their critical section
		sem_wait(&MUTEX);

		// critical section
		Ccounter += 1;
		value = dequeue_item();
		printf(ANSI_COLOR_GREEN "Consumer %-2d has consumed %d\n", args->id, value);

		// end critical section
		sem_post(&MUTEX);
		// increments empty counter
		sem_post(&EMPTY);

		sleep(Ctime);
	}
}

// over consumption thread function
void *consumerOver(void *arg) {

	// uses struct in order to print out nice thread ids
	struct ThreadArgs* args = (struct ThreadArgs*) arg;
	int value;

	for (int i = 0; i < totalConsumption; i++) {
	
		// blocks if buffer is empty, otherwise decrements data counter
		sem_wait(&DATA);
		// blocks if any other producer/consumer is in their critical section
		sem_wait(&MUTEX);

		// critical section
		Ccounter += 1;
		value = dequeue_item();
		printf(ANSI_COLOR_GREEN "Consumer %-2d has consumed %d\n", args->id, value);

		// end critical section
		sem_post(&MUTEX);
		// increments empty counter
		sem_post(&EMPTY);

		sleep(Ctime);
	}
}

// prints the date/time nicely
void printTime() {

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	printf(ANSI_COLOR_RESET "Current Time: %s\n", asctime(tm));
}

// prints out both produce and consumer arrays just like in same code
// checks if the arrays are equal at the end
void checkEqual() {

	int totalItem = (P * X);
	printf("Producer Array | Consumer Array\n");
	for (int i = 0; i < counter; i++) {

		printf("%-14d | %d\n", producerArray[i], consumerArray[i]);
	}
	if (memcmp(producerArray, consumerArray, sizeof(producerArray)) == 0) {

		printf("Producer and Consumer Arrays Match!!!\n");
	} else {

		printf("Programmer messed up..\n");
	}
}

// prints info just like in sample code
void printInfo() {

	printf("Buffer Size: %d\n", N);
	printf("Number of Producers: %d\n", P);
	printf("Number of Consumers: %d\n", C);
	printf("Number of items Produced by each Producer: %d\n", X);
	printf("Number of items consumed by each Consumer: %d\n", consumeAmount);
	if (consumeOver != 0) {

		printf("Overconsumption: true\n");
		printf("Overconsumption Amount : %d\n", totalConsumption);
	} else {

		printf("Overconsumption: false\n");
	}
	printf("Time each Producer Sleeps (seconds): %d\n", Ptime);
	printf("Time each Consumer Sleeps (seconds): %d\n", Ctime);
}

int main(int argc, char** argv) {

	// tried to make arguments as macros(#define), it would work a few times then stop
	// made them global instead, much more stable
	N     = atoi(argv[1]);
	P     = atoi(argv[2]);
	C     = atoi(argv[3]);
	X     = atoi(argv[4]);
	Ptime = atoi(argv[5]);
	Ctime = atoi(argv[6]);

	printTime();

	// starts timer to aquire total runtime
	time_t begin = time(NULL);

	// equations that deal with over consumption
	consumeOver = (P * X) % C; 
	consumeAmount = (P * X) / C;
	totalConsumption = consumeAmount + consumeOver;

	printInfo();

	// initializes the semaphores 
	sem_init(&MUTEX, 0, 1);
	sem_init(&DATA, 0, 0);
	sem_init(&EMPTY, 0, N);
	
	// thread creation starts
	int totalPC = P + C; 
	struct ThreadArgs threads[totalPC];
	int ret_val;
	// creates producer threads
	for (int i = 0; i < P; i++) {

		// gives thread id
		threads[i].id = i + 1;
		ret_val = pthread_create(&threads[i].tid, NULL, producer, (void *)&threads[i]);
		// checks if thread is created successfully
		if (ret_val < 0) {

            perror("Error creating thread..");
            return -2;
        }	
	}
	// creates consumer threads
	if (consumeOver != 0) {

		for (int i = P; i < totalPC - 1; i++) {

			// gives thread id
			threads[i].id = i + 1 - P;
			ret_val = pthread_create(&threads[i].tid, NULL, consumer, (void *)&threads[i]);
			// checks if thread is created successfully
			if (ret_val < 0) {

  	          perror("Error creating thread..");
  	          return -2;
 	       }	
		}
		threads[totalPC - 1].id = totalPC - P;
		ret_val = pthread_create(&threads[totalPC - 1].tid, NULL, consumerOver, (void *)&threads[totalPC - 1]);
		if (ret_val < 0) {

  	          perror("Error creating thread..");
  	          return -2;
 	    }	
	} else {

		for (int i = P; i < totalPC; i++) {

			// gives thread id
			threads[i].id = i + 1 - P;
			ret_val = pthread_create(&threads[i].tid, NULL, consumer, (void *)&threads[i]);
			// checks if thread is created successfully
			if (ret_val < 0) {

  	          perror("Error creating thread..");
  	          return -2;
 	       }

		}
	}
 
 	// joins the threads, copy from pthreadracer
	for (int i = 0; i < totalPC; i++) {

        ret_val = pthread_join(threads[i].tid, NULL);
        if (ret_val) {
            perror("Error joining thread..");
            return -3;
        }
    }

    printTime();
	checkEqual();

	// prints total runtime
	time_t end = time(NULL);
	printf("Total Runtime: %ld seconds\n", (end - begin));
}
