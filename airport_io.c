#define _CRT_SECURE_NO_WARNINGS
#include "Airport.h"

int** allocate_gate_matrix(int rows, int cols) {
    int** matrix = (int**)malloc(rows * sizeof(int*));
    if (matrix == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    for (int i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
        if (matrix[i] == NULL) {
            printf("Memory allocation failed!\n");
            exit(1);
        }
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}

void print_terminal_status(int** matrix, int rows, int cols) {
    printf("\n--- Terminal Gates Status ---\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] == 0) {
                printf("[ EMPTY ] ");
            }
            else {
                printf("[ FLT%d ] ", matrix[i][j]);
            }
        }
        printf("\n");
    }
    printf("-----------------------------\n\n"); 
}

void free_gate_matrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}
void load_config(const char* filename, int* rows, int* cols, int* runways, int* num_planes) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open config file %s!\n", filename);
        exit(1);
    }

    char dummy[50];

    fscanf(file, "%s %d", dummy, rows);
    fscanf(file, "%s %d", dummy, cols);
    fscanf(file, "%s %d", dummy, runways);
    fscanf(file, "%s %d", dummy, num_planes);

    fclose(file);

    printf("Config loaded successfully:\n");
    printf("  - Terminal Gates Matrix: %d x %d\n", *rows, *cols);
    printf("  - Total Runways: %d\n", *runways);
    printf("  - Simulated Planes: %d\n\n", *num_planes);
}

void write_to_log(const char* message) {
    FILE* file = fopen("airport_log.txt", "a");
    if (file == NULL) {
        printf("Error: Could not open log file!\n");
        return;
    }

    fprintf(file, "%s\n", message);
    fclose(file);
}
int assign_gate(int** matrix, int rows, int cols, int plane_id) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] == 0) {
                matrix[i][j] = plane_id;
                printf("Flight %d assigned to Gate [%d,%d] (Priority Row %d)\n", plane_id, i, j, i);
                return 1;
            }
        }
    }
    printf("Alert: Terminal is FULL! Flight %d must wait in the air.\n", plane_id);
    return 0; 
}

void release_gate(int** matrix, int rows, int cols, int plane_id) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] == plane_id) {
                matrix[i][j] = 0; 
                printf("Gate released for flight %d.\n", plane_id);
                return; 
            }
        }
    }
    printf("Flight %d not found in any gate!\n", plane_id);
}