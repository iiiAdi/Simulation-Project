#ifndef AIRPORT_H
#define AIRPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ARRIVING,
    LANDING,
    AT_GATE,
    DEPARTING
} PlaneState;

typedef enum {
    EVENT_PLANE_ARRIVAL,
    EVENT_RUNWAY_FREE,
    EVENT_GATE_FREE
} EventType;

typedef struct Plane {
    int id;
    char flight_num[10];
    PlaneState state;
    double arrival_time;
    int fuel_level;
    struct Plane* next;
} Plane;

typedef struct SimulationEvent {
    double event_time;
    EventType type;
    int plane_id;
    struct SimulationEvent* next;
} SimulationEvent;

typedef struct {
    int** gate_matrix;
    int matrix_rows;
    int matrix_cols;

    int total_runways;
    int busy_runways;

    SimulationEvent* fel_head;

    Plane* queue_head;
    Plane* queue_tail;
} Airport;

#endif