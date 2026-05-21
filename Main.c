#define _CRT_SECURE_NO_WARNINGS
#include "Airport.h"
#include <time.h>

void init_airport_defaults(Airport* airport) {
    airport->fel_head = NULL;
    airport->queue_head = NULL;
    airport->queue_tail = NULL;
    airport->busy_runways = 0;

    airport->gate_matrix = NULL;
    airport->matrix_rows = 0;
    airport->matrix_cols = 0;
    airport->total_runways = 0;
}

int main() {
    srand((unsigned int)time(NULL));

    Airport my_airport;
    init_airport_defaults(&my_airport);

    int rows = 0, cols = 0, runways = 0, num_planes = 0;;

    printf("Reading configuration file...\n");
    load_config("config.txt", &rows, &cols, &runways, &num_planes);

    my_airport.matrix_rows = rows;
    my_airport.matrix_cols = cols;
    my_airport.total_runways = runways;

    my_airport.gate_matrix = allocate_gate_matrix(rows, cols);

    print_terminal_status(my_airport.gate_matrix, rows, cols);

    printf("Scheduling initial flight arrivals into FEL...\n");
    double next_arrival_time = 0.0;
    for (int i = 0; i < num_planes; i++) {
        next_arrival_time += (rand() % 15) + 1;

        int generated_plane_id = 100 + (i + 1);

        insert_event(&my_airport, EVENT_PLANE_ARRIVAL, next_arrival_time, generated_plane_id);
    }

    run_simulation(&my_airport, 500.0);

    printf("\nCleaning up system memory...\n");
    free_gate_matrix(my_airport.gate_matrix, rows);

    return 0;
}