#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "api.h"


void report_accesses_and_clear(DB * db, uint32_t expected) {
    uint32_t counted_accesses = db_total_accesses(db);
    uint32_t individual_accesses = db_sum_individual_accesses(db);
    printf("%20s\t%20s\t%20s\n", "Expected", "Sum individual", "Total");
    printf("%20u\t%20u\t%20u\n", expected, individual_accesses, counted_accesses);
    db_clear_accesses(db);
}


int main(int argc, char **argv) {

    int N = 24; // N default value = 24
    if (argc >= 2) {
        N = atoi(argv[1]); // read N value from arguments
    }
    printf("N = %d\n", N);
    uint64_t num_elements = 1UL << N; // num_elems = 2^N

    // exit if we didn't get at least 64 elements
    if(num_elements < 64) {
        printf("Common, ask for something bigger please...\n");
        return 1;
    }
    printf("Num elements: %lu\n", num_elements);

    printf("Size of Person: %lu bytes\n", sizeof(Person));

    uint64_t database_size = num_elements * sizeof(Person);

    // exit if we requested to much to avoid consuming all RAM.
    if (database_size > 4UL * 1024UL * 1024UL * 1024UL) {
        printf("More that 4GB requested... be reasonable please.\n");
        return 1;
    }

    // print data size
    if (database_size > 1024 * 1024) {
        printf("Total data size: %lu Mo\n", database_size / (1024UL * 1024UL) );
    } else {
        printf("Total data size: %lu Ko\n", database_size / (1024UL) );
    } 

    // create randomized persons for the database
    Person* ps = generate_random_persons(num_elements); 
    

    // initialize database with the generated persons
    DB *db = db_init(ps, num_elements);

    // now that the database is populated, delete the generated persons
    // they should only remain in the database
    free(ps);

    // set the number of accesses to measure to 10M
    uint64_t num_lookups = 1024 * 1024 * 10;

    // // do 'num_lookups' lookups into the database, each time increasing 
    // // the looked-up ID by 1.
    // START_CHRONO(sequential_lookups);
    // for(uint32_t i = 0 ; i < num_lookups ; i++) {
    //     db_get(db, i % num_elements);
    // }
    // STOP_CHRONO(sequential_lookups, num_lookups);

    // // look up random
    // // do 'num_lookups' lookups into the database, each time accessing 
    // // a random ID
    // START_CHRONO(random_lookups);
    // for(uint32_t i = 0 ; i < num_lookups ; i++) {
    //     db_get(db, RANDOM_NUMBER(i) % num_elements);
    // }
    // STOP_CHRONO(random_lookups, num_lookups);



    printf("\n======= SIMD =======\n");

    int age;
    START_CHRONO(max_age);
    age = db_max_age(db);
    STOP_CHRONO(max_age, num_elements);

    float dist_closest;
    START_CHRONO(min_dist);
    dist_closest = db_min_dist(db, -1.1, -2.1);
    STOP_CHRONO(min_dist, num_elements);

    pid id_closest;
    START_CHRONO(closest);
    id_closest = db_closest(db, -1.1, -2.1);
    STOP_CHRONO(closest, num_elements);


    printf("Results of queries: (max-age: %d) (min-dist: %.2f) (closest: %ld) \n", age, dist_closest, id_closest);


    int num_threads = 4; 

    printf("\n======= Multithreading (%d threads) =======\n", num_threads);

    // get requests that must be handled    
    uint64_t num_requests = 1024 * 1024 * 10;
    Request* requests = generate_random_requests(num_elements, num_requests);

    START_CHRONO(many_requests);
    db_process_many_requests(db, requests, num_requests);
    STOP_CHRONO(many_requests, num_requests);
    report_accesses_and_clear(db, num_requests);

    // // 3.B: décommentez pour activer
    // START_CHRONO(many_requests_par);
    // db_process_many_requests_parallel(db, requests, num_requests, num_threads, false);
    // STOP_CHRONO(many_requests_par, num_requests);
    // report_accesses_and_clear(db, num_requests);


    // // 3.I décommenter pour activer
    // START_CHRONO(many_requests_par_split);
    // db_process_many_requests_parallel(db, requests, num_requests, num_threads, true);
    // STOP_CHRONO(many_requests_par_split, num_requests);
    // report_accesses_and_clear(db, num_requests);
    
    



    printf("\nExecution successful, exiting.\n");

    // release memory before quitting
    // deactivated to enable messing with the return value of malloc to force alignment
    // db_free(db);
    return 0;
}
