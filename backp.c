#include "helac.h"
#include <stdlib.h>

#define LOG_TRACE 0
#include "clog.h"
#include <bits/pthreadtypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>


// currently we load them into a 4d vector; tecnically, zmm could fit 32 values at the same time,
// but id rather stick to ymm, holding 16 values of 16bit integers. (I dont have AVX512 T-T)
// then we use it to essentially compute all sets at once, in one go.
// we use threads to partition the computation of subsets and raw assembly vectorized shit to compute the backpack
// these are global variables for easier linking with assembly.
u16 sizes[64][16] = {0};

u16 values[64][16] = {0};

u16 capacity = 0;
u16 item_count = 0;
u16 backpack_count = 0;

u8 thread_count = 12;

typedef struct {
	u64 subsets[16];
	u16 values[16];
	bool complete;
} ThreadResults;

typedef struct {
	u64 start, end;
	ThreadResults* results;
} ThreadParams;


extern u32 find_best_set(u64 i_min, u64 i_max, void* output);

void* find_best_subsets(void* starting_params) {
	ThreadParams params = *(ThreadParams*)starting_params;
	dlog("Thread %08lx: (%16lx - %16lx) started.\n", pthread_self(), params.start, params.end);
	find_best_set(params.start, params.end, params.results);
	params.results->complete = true;
	dlog("Thread %08lx: (%16lx - %16lx) finished.\n", pthread_self(), params.start, params.end);
	return NULL;
}

Error(SYNTAX_FAIL);
MakeError(SYNTAX_FAIL);

Error(DATA_FAIL);
MakeError(DATA_FAIL);

Optional(void) tokenize(FILE* file) {
	if(fscanf(file, "length - %hu, capacity %hu\n", &item_count, &capacity) != 2) return Err(void, SYNTAX_FAIL);
	if(item_count > 64) return Err(void, DATA_FAIL);

	u32 set_num = 0;
	char buf[256];
	u16* destination;

	backpack_count = 0;

	for(u8 i = 0; i < 16; i++) {
		if(fscanf(file, "dataset %u:\n", &set_num) != 1)
			break;
		i8 flag = 0;

sec:
		if(fscanf(file, "%s = ", buf) != 1)
			return Err(void, SYNTAX_FAIL);

		if(!strcmp(buf, "vals")) {
			destination = (u16*)values + i;
			tlog("Values\n");

			fscanf(file, "{");
			for(u8 j = 0; j < item_count; j++) {
				fscanf(file, "%hu, ", destination);
				tlog("\t%hu \n", *destination);
				destination += 16;
			}
			fscanf(file, " }\n");

			flag++;
		} else if(!strcmp(buf, "sizes")) {
			destination = (u16*)sizes + i;
			tlog("Sizes\n");

			fscanf(file, "{");
			for(u8 j = 0; j < item_count; j++) {
				fscanf(file, "%hu, ", destination);
				tlog("\t%hu \n", *destination);
				destination += 16;
			}
			fscanf(file, " }\n");

			flag++;
		} else {
			return Err(void, SYNTAX_FAIL);
		}

		if(flag != 2) goto sec;

		dlog("Parsed set %d\n", i + 1);
		backpack_count++;
	}
	return Ok(void);
}

i32 usage(cstr name) {
	printf("Usage: %s DATA [THREADS]\n", name);
	return 1;
}

ThreadResults main_brute() {
	pthread_t threads[thread_count];
	ThreadParams params[thread_count];
	ThreadResults results[thread_count];

	memset(threads, 0, sizeof(pthread_t) * thread_count);
	memset(params, 0, sizeof(ThreadParams) * thread_count);
	memset(results, 0, sizeof(ThreadResults) * thread_count);

	u64 chunk = (1L << item_count) / thread_count;

	for(u8 i = 0; i < thread_count; i++) {
		params[i].start = i * chunk;
		params[i].end = (i + 1) * chunk;
		params[i].results = &results[i];

		pthread_create(&threads[i], NULL, find_best_subsets, &params[i]);
	}

	ThreadResults best_results = {0};

	for(u8 i = 0; i < thread_count; i++) {
		pthread_join(threads[i], NULL);
	}


	for(u8 i = 0; i < thread_count; i++) {
		if(!results[i].complete) continue;

		for(i8 j = 0; j < 16; j++) {
			if(results[i].values[j] <= best_results.values[j]) continue;
			best_results.values[j] = results[i].values[j];
			best_results.subsets[j] = results[i].subsets[j];
		}
	}


	return best_results;
}

typedef struct {
    f32 ratio;
    i32 item_id;
} BackpackItem;

int compare_items(const void* arg1, const void* arg2) {
    BackpackItem* a = (BackpackItem*)arg1;
    BackpackItem* b = (BackpackItem*)arg2;
    return (a->ratio < b->ratio) - (a->ratio > b->ratio);
}

ThreadResults main_heur() {
    ThreadResults results = {0};

    for(u8 j = 0; j < backpack_count; j++) {
        BackpackItem items[item_count];

        for(u8 i = 0; i < item_count; i++) {
            items[i] = (BackpackItem) {
                .item_id = i,
                .ratio = (f32)values[i][j] / sizes[i][j]
            };
        }

        qsort(items, item_count, sizeof(BackpackItem), compare_items);

        u32 current_size = 0;
        u32 current_value = 0;
        u64 current_set = 0;

        for(u8 i = 0; i < item_count; i++) {
            u32 id = items[i].item_id;

            if(current_size + sizes[i][j] <= capacity) {
                current_size += sizes[i][j];
                current_value += values[i][j];
                current_set |= 1 << i;
            }
        }

        results.subsets[j] = current_set;
        results.values[j] = current_value;

        // ilog("Backpack %d:\n", j);
        // for(u8 i = 0; i < item_count; i++) {
        //     ilog("\t%d: %f\n", items[i].item_id, items[i].ratio);
        // }
    }

    results.complete = true;
	return results;
}

i32 main(i32 argc, cstr* argv) {
	memset(sizes, 0xFF, 64 * 16 * sizeof(u16));

	if(argc < 2) {
		return usage(argv[0]);
	}

	FILE* input = fopen(argv[1], "r");
	if(!input) {
		wlog("Could not open the file.\n");
		return usage(argv[0]);
	}

	Optional(void) result = tokenize(input);
	if(result.error == ERROR_DATA_FAIL) {
		wlog("Could not load the file! Invalid values detected.\n");
		return usage(argv[0]);
	} else if(result.error == ERROR_SYNTAX_FAIL) {
		wlog("Could not load the file! Syntax invalid.\n");
		return usage(argv[0]);
	}
	fclose(input);

	if(argc == 3) {
		thread_count = atoi(argv[2]);
	}

	if(thread_count == 0) thread_count = 1;

	ilog("Using %02u threads.\n", thread_count);

	ThreadResults results;
#ifdef MAIN
	results = MAIN();
#else
	ilog("meow");
#endif
	for(i8 j = 0; j < backpack_count; j++) {
		ilog("Backpack %2d: =======================\n", j + 1);
		// ilog("%.*lb \n", item_count, results.subsets[j]);
		ilog("Total value = %d; Item count = %d\n", results.values[j], __builtin_popcount(results.subsets[j]));

        for(u8 i = 0; i < item_count; i++) {
            if((1 << i) & results.subsets[j]) {
                ilog("\t%2u (size = %3u, value = %3u)\n", i, sizes[i][j], values[i][j]);
            }
        }
	}
}
