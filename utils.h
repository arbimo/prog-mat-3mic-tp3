#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include "api.h"


#define STRING_SIZE 16
#define NUM_REPS 10
#define WARM_UP_ROUNDS 3

// Macros to help measure the elapsed time.    
#define START_CHRONO(X) \
double X = 0.0; \
for(int _n_rep = 0 ; _n_rep<NUM_REPS; _n_rep++) { \
    double start = FLA_Clock();
#define STOP_CHRONO(X, OPS) \
    double dur = FLA_Clock() - start; \
    if(_n_rep >= WARM_UP_ROUNDS) X += ((double) dur) / ((double) NUM_REPS - WARM_UP_ROUNDS); \
} \
printf("%-30s: %10.4lf ms\t %6.2lf ns / op\n", (#X), (X) * 1000.0, (X) / (double) (OPS) * 10e9);

// A global variable used when FLA_Clock_helper() is defined in terms of
// clock_gettime()/gettimeofday().
double gtod_ref_time_sec = 0.0;

/** Get a time in nanoseconds. Implementation from flame graphs. */
double FLA_Clock()
{
    double the_time, norm_sec;
    struct timespec ts;

    clock_gettime( CLOCK_MONOTONIC, &ts );

    if ( gtod_ref_time_sec == 0.0 )
        gtod_ref_time_sec = ( double ) ts.tv_sec;

    norm_sec = ( double ) ts.tv_sec - gtod_ref_time_sec;

    the_time = norm_sec + ts.tv_nsec * 1.0e-9;

    return the_time;
}

#define NUM_NAMES 4
char first_names[NUM_NAMES][STRING_SIZE] = { "Tom", "Arthur", "Harry", "Felix"};
char last_names[NUM_NAMES][STRING_SIZE] = { "Bombadil", "Potter", "Jedusor", "Dumbledore" };


#define NUM_RANDS (1024 * 1024 * 128)
// array of random numbers. Those are pregenrated because generating random number is expensive
// and might impact benchmark time.
int rands[NUM_RANDS];
#define RANDOM_NUMBER(i) rands[(i) % NUM_RANDS]

#define loc_from_int(i) ((float) (i % (1024)))



/** Generates randomized data to load into the database */
Person* generate_random_persons(int num_elements) {
    Person * ps = malloc(sizeof(Person) * num_elements);
    srand(0);
    for(int i=0; i<NUM_RANDS ; i++) {
        rands[i] = rand();
    }
    for(int i=0 ; i < num_elements ; i++) {
        int x = rands[i % NUM_RANDS];
        Person p;
        p.first_name = first_names[x % NUM_NAMES];
        p.last_name = last_names[(x >> 10) % NUM_NAMES];
        p.age = x % 128;
        p.latitude = loc_from_int(x);
        p.longitude = loc_from_int(x / 1024);
        p.male = (x & (1 << 20)) ? true : false; // true if the 20th bit is 1
        p.wizard = (x % 8) == 0; // 1 wizard in 8 persons
        ps[i] = p;
    }

    return ps;
}


Request * generate_random_requests(int num_persons, int num_requests) {
    uint64_t * results = malloc(sizeof(uint64_t) * num_persons);
    void * result_slot = malloc(64);
    // void * result_slot = NULL;
    
    Request * array = malloc(sizeof(Request) * num_requests);
    for(int i=0; i<num_requests; i++) {
        int x = rands[i % NUM_RANDS] % 100;
        Request r;
        if (x< 70) {
            r.op = GET_DIST;
        } else if (x <= 80) {
            r.op = GET_AGE;
        } else {
            r.op = SET_AGE;
            r.value = 20;
        }
        // r.person_id = rand() % num_persons; 
        // r.person_id = (rand() & 0xFFFF00FF) % num_persons;
         r.person_id = (rand() & 0xFFFFFF00) % num_persons;
        // r.person_id = rands[rand() % 10000] % num_persons; 
        r.result = (void *) &results[r.person_id];
        array[i] = r;
    }
    return array;
}


#endif
