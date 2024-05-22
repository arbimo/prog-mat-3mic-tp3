#include "api.h"

#include "simd.h"  // helper for simd operations

#include <pthread.h>     // for multi-threading
#include <stdatomic.h>   // for atomic operations


/** Allocate the given number of bytes such that the 
 *  returned address is always aligned on 32 bytes */
void * malloc_aligned(size_t size) {
    // round up the allocated size to a multiple of 32
    size_t aligned_size = size + 32 - (size % 32);
    return aligned_alloc(32, aligned_size);
}



typedef struct DB {
    char **first_names;
    char **last_names;
    uint32_t *age;
    char *male;
    char *wizard;
    float * latitudes;
    float * longitudes;

    // an array indicating for each person how many times it was accessed.
    uint32_t *access_counts;
    // total number of accesses regardless of the person
    uint32_t global_access_count;

    // number of persons in the database
    int size; 
} DB;


DB* db_init(Person *persons, int len) {
    DB * db = malloc(sizeof(DB));
    db->first_names = malloc_aligned(sizeof(char *) * len);
    db->last_names = malloc_aligned(sizeof(char *) * len);
    db->age = malloc_aligned(sizeof(uint32_t) * len);
    db->male = malloc_aligned(sizeof(char) * len);
    db->wizard = malloc_aligned(sizeof(char) * len);
    db->latitudes = malloc_aligned(sizeof(float) * len);
    db->longitudes = malloc_aligned(sizeof(float) * len);
    db->access_counts = malloc_aligned(sizeof(uint32_t) * len);
    db->global_access_count = 0;
    db->size = len;
    for(int i = 0 ; i<len ; i++) {
        db->first_names[i] = persons[i].first_name;
        db->last_names[i] = persons[i].last_name;
        db->age[i] = persons[i].age;
        db->male[i] = persons[i].male;
        db->wizard[i] = persons[i].wizard;
        db->latitudes[i] = persons[i].latitude;
        db->longitudes[i] = persons[i].longitude;
        db->access_counts[i] = 0;
    }
    return db;
}

/** Free all memory from the database */
void db_free(DB * db) {
    free(db->first_names);
    free(db->last_names);
    free(db->age);
    free(db->male);
    free(db->wizard);
    free(db->latitudes);
    free(db->longitudes);
    free(db->access_counts);
    free(db);
}

Person db_get(DB *db, pid i) {
    Person p = {
            .first_name = db->first_names[i],
            .last_name = db->last_names[i],
            .age = db->age[i],
            .male = db->male[i],
            .wizard = db->wizard[i],
            .latitude = db->latitudes[i],
            .longitude = db->longitudes[i]
    };
    return p;
}




/**********  SIMD  ***********/


/** Returns the age of the oldest person. */
uint32_t db_max_age(DB * db) {
    uint32_t * ages = db->age;
    int size = db->size;
    uint32_t max = 0;

    for(int index = 0; index<size; index++) {
        uint32_t age = ages[index];
        max = max < age ? age : max;
    }
    return max;
}

/** Returns the age of the oldest person. */
uint32_t db_max_age_simd(DB * db) {
    // the array of ages
    uint32_t * ages = db->age;

    // the number of persons in the database (size of the array)
    int size = db->size;
    
    // a vector containing 8 maximum values
    // the first int will contain the max of ages[0], ages[8], ages[16], ...
    // the second int will contain the max of ages[1], ages[9], ages[17], ...
    // ...
    // Initally all ints in the vector are set to 0
    __m256i maxs = _mm256_set1_epi32(0);

    for(int index = 0; index<size; index += 8) {
        // load the 8 next 32bits integers
        __m256i candidates =  _mm256_load_si256((__m256i *) &ages[index]);

        // compute the pairwise maxs of both vectors and store the result in "maxs"
        maxs = _mm256_max_epu32(maxs, candidates);
    }

    // get the maximum of the 8 integers that are stored in "maxs"
    uint32_t max = 0;
    for(int i=0; i<8; i++) {
        uint32_t candidate = extract_uint32(maxs, i);
        max = candidate > max ? candidate : max; 
    }
    return max;
}


/** Find the distance of the given coordinates to the closest person. */
float db_min_dist(DB *db, float lat, float lon) {
    float min_dist_squared = 1.0 / 0.0; // infinity
    int size = db->size;
    // latitude and longitude arrays
    float * lats = db->latitudes;
    float * lons = db->longitudes;
    for(int i = 0 ; i<size ; i++) {
        float dx = lats[i] - lat;
        float dy = lons[i] - lon;
        float dx_squared = dx * dx;
        float dy_squared = dy * dy;
        float dist_squared = dx_squared + dy_squared;
        
        // min_dist_squared = MIN(min_dist_squared, dis_squared)
        if (dist_squared < min_dist_squared) {
            min_dist_squared = dist_squared;
        }
    }
    return sqrtf(min_dist_squared);
}

/** TODO: work in progress implementation of a SIMD version of the db_min_dist query */
float db_min_dist_simd(DB *db, float lat, float lon) {
    printf("\n======== work in progress simd implementation =======\n");
    int size = db->size;
    float * latitudes = db->latitudes;
    float * longitudes = db->longitudes;

    __m256 base_lat = _mm256_set1_ps(lat);
    print_vec("base lat", base_lat);

    // for now we just work on the first 8 values
    __m256 lats = _mm256_load_ps(&latitudes[0]);
    print_vec("latitudes", lats);

    __m256 dxs = _mm256_sub_ps(lats, base_lat);
    print_vec("dxs", dxs);

    printf(" =======================\n");


    // TODO: at the end, we should get the minimal value in the vector of
    // the 8 minimal squared distances and return its squared root
    // float min = min_float_in_vector(...);
    // return sqrtf(min);
    exit(1);
}


/** Find the closest person to the given coordinates */
pid db_closest(DB *db, float lat, float lon) {
    float min_dist = 1.0 / 0.0; // infinity
    int closest = -1;
    int size = db->size;
    float * lats = db->latitudes;
    float * lons = db->longitudes;
    for(int i = 0 ; i<size ; i++) {
        float dx = lat - lats[i];
        float dy = lon - lons[i];
        float dist = dx * dx + dy * dy;
        if(dist < min_dist) {
            min_dist = dist;
            closest = i;
        }
    }
    return closest;
}






/**************** THREADS ***************/

/** A set of parameters that are given to each thread when it is started. */
typedef struct ThreadParams {
    // Database on which to operate.
    DB * db; 
    // An array of all requests that must be handled collectively by all threads. 
    // The thread will typically carry out only a subset of those.
    Request * requests;
    // Number of requests in the array.
    int num_requests;
    // ID of the thread. If there are N threads running, the number should be in [0, N[
    int thread_id;
    // Total number of threads running.
    int num_threads;
} ThreadParams;


/** Processes a request on a particulat database.
 * 
 * Arguments:
 *  - db: database on which we should execute the request
 *  - req: request to perform, containing an operation, a person ID, and address to write the result to.
 *  - atomic: if true the increment to counters should be done in an atomic way (needed for 3.X)
 */ 
void db_process_single_request(DB * db, Request * req, bool atomic) {
    // Operation to carry out
    enum Op op = req->op;
    // ID of the person on which to carry out the operation
    pid id = req->person_id;
    
    // execute the query
    if(op == GET_AGE) {
        // get the age of the person and write to the result address
        uint32_t age = db->age[id];
        uint32_t * result = req->result;
        if (result != NULL) {
            (*result) = age;
        }
    } else if (op == SET_AGE) {
        // set the age of the person to the value present in the request.
        db->age[id] = req->value; 
    } else if (op == GET_SEX) {
        // get the sex of the person and write it to the result address
        char sex = db->male[id];
        char * result = req->result;
        if (result != NULL) {
            (*result) = sex;
        }
    } else if (op == GET_DIST) {
        // compute the distance of the person to the (0,0) reference point 
        // and write the result
        float lat = db->latitudes[id];
        float lon = db->longitudes[id];
        float dist = sqrtf(lat * lat + lon * lon);
        float * result = req->result;

        if (result != NULL) {
            (*result) = dist;
        }
    } else {
        printf("Error unsupported operation\n");
        exit(1);
    }

    // increase the counters of executed requests
    if(!atomic) { 
        // initial version, valid for single thread situations
        db->access_counts[id]++;
        db->global_access_count++;
    } else {
        // atomic version
        // TODO 3.E: increment both counters atomically
        printf("ERROR: unimplemented block in db_process_single_request\n");
        exit(1); 
    }
}

/** Processes an array of requests on a single thread. */
void db_process_many_requests(DB * db, Request * req_array, int num_requests) {
    for(int i=0; i<num_requests; i++) {
        db_process_single_request(db, &req_array[i], false);
    }
}

/** This is the handler run on each started thread of the "base" parallel model. 
 *  It must handle a subset of the requests and exit.
 *  Its ThreadParams parameters contains:
 *   - the requests that must be handle collectively on all threads
 *   - the database on which to perform the requests
 *   - the total number of threads
 *   - the id of this thread in [O, num_threads[]
*/
void * thread_process_requests_base(void * thread_params) {
    // Cast the parameters passed to this thread to the right type
    ThreadParams params = *((ThreadParams *) thread_params);

    // A thread processes the i-th request if:  params.thread_id == i % params.num_threads
    // For instance, if we have four threads:
    // - thread 0 will process requests 0, 4, 8, ...
    // - thread 1 will process requests 1, 5, 9, ...
    // - ...
    for(int i=params.thread_id; i<params.num_requests; i += params.num_threads) {
        db_process_single_request(params.db, &params.requests[i], false);
    }

    // nothing to return
    return NULL;
}

/** This is the handler run on each started thread of the "split" parallel model. 
 *  It must handle all request that are on a the persons the running thread 
 *  is responsible for.
*/
void * thread_process_requests_split(void * thread_params) {
    // Cast the parameters passed to this thread to the right type
    ThreadParams params = *((ThreadParams *) thread_params);

    // 3.I: use params to determine which persons this thread is responsible of
    //      depending on the thread_id

    
    for(int i=0; i<params.num_requests; i++) {
        // 3.I: handle the i-th request if it is on a person we are responsible of
    }
    

    return NULL;
}


/** Process an array of requests in parallel.
 * 
 * Parameters:
 *  - db: database on which the requests must be exececuted
 *  - request_array: pointer to the first element of an array of requests
 *  - num_requests: number of requests in the array
 *  - num_threads: number of threads that should be started to process the requests
 *  - split: a boolean value that indicates how to divide the work between the threads.
 */
void db_process_many_requests_parallel(DB * db, Request * request_array, int num_requests, int num_threads, bool split) {
    // array to keep trace of the metadata of each thread, populate by "pthread_create"
    pthread_t * threads = malloc(sizeof(pthread_t) * num_threads);
    for (int i=0; i<num_threads; i++) {
        // initialize some data that will passed to the thread on its creation
        ThreadParams * params = malloc(sizeof(ThreadParams));
        params->thread_id = i; // id of the thread
        params->num_threads = num_threads; // total number of threads
        params->db = db; // database on which to perform the requests
        params->requests = request_array; // first element of the request array
        params->num_requests = num_requests; // number of requests in the array

        // start all threads 
        if (!split) {
            // base version: create a thread that will run the "thread_process_request_base" function.
            // give it, the "params" object we just created so that it can know its id
            // and the set of request to process
            if (pthread_create (&threads[i], NULL, thread_process_requests_base, params) < 0) {
                fprintf (stderr, "pthread_create error for thread %d\n", i);
                exit (1);
            }
        } else {
            // unused until question 2.X...
            // split version: same as above but run the "thread_process_requests_split" instead.
            if (pthread_create (&threads[i], NULL, thread_process_requests_split, params) < 0) {
                fprintf (stderr, "pthread_create error for thread %d\n", i);
                exit (1);
            }
        }
    }

    // wait for all threads to finish before exiting the function.
    void * ret;
    for (int i=0; i<num_threads; i++) {
        pthread_join(threads[i], &ret);
    }
}


/********** ACCESSES ************/
//  Utility functions to count and clear the accesses in the database

uint32_t db_total_accesses(DB * db) {
    return db->global_access_count;
}

uint32_t db_sum_individual_accesses(DB * db) {
    uint32_t * accesses = db->access_counts;
    int len = db->size;
    uint32_t count = 0;
    for (int i=0; i<len; i++) {
        count += accesses[i];
    }
    return count;
}

void db_clear_accesses(DB * db) {
    uint32_t * accesses = db->access_counts;
    int len = db->size;
    for (int i=0; i<len; i++) {
        accesses[i] = 0;
    }
    db->global_access_count = 0;
}
