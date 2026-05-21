// IMPORTS
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include "Airport.h"

void insert_event(Airport* airport, EventType type, double eventTime, int planeId) {
	SimulationEvent* newEvent = (SimulationEvent*)malloc(sizeof(SimulationEvent));
	if (newEvent == NULL) {
		fprintf(stderr, "Error: Memory Allocation failed for the new event.\n");
		return;
	}

	newEvent->type = type;
	newEvent->event_time = eventTime;
	newEvent->plane_id = planeId;
	newEvent->next = NULL;

	if (airport->fel_head == NULL || eventTime < airport->fel_head->event_time) {
		newEvent->next = airport->fel_head;
		airport->fel_head = newEvent;
		return;
	}

	SimulationEvent* current = airport->fel_head;
	while (current->next != NULL && current->next->event_time < eventTime) {
		current = current->next;
	}

	newEvent->next = current->next;
	current->next = newEvent;
}

void run_simulation(Airport* airport, double max_sim_time) {
    // Time check
    double current_time = 0.0;
   
    // Log Messages
    int total_arrivals = 0;
    int total_landed = 0;
    int total_departed = 0;
    int total_delayed_in_air = 0;

    FILE* log_file = fopen("airport_log.txt", "w");

    printf("=== Simulation Started ===\n");
    if (log_file != NULL) {
        fprintf(log_file, "=== Simulation Started ===\n");
    }

    while (airport->fel_head != NULL && current_time < max_sim_time) {
        SimulationEvent* current_event = airport->fel_head;

        if (current_event == NULL) {
            fprintf(stderr, "Error: Found a NULL pointer in the Event List.\n");
            break;
        }

        // Update time and the list head
        current_time = current_event->event_time;
        airport->fel_head = airport->fel_head->next;

        switch (current_event->type) {
        case EVENT_PLANE_ARRIVAL:
            total_arrivals++;
            printf("[T = %.2f]: Plane ID %d arrived at airspace.\n", current_time, current_event->plane_id);
            if (log_file != NULL) {
                fprintf(log_file, "[T = %.2f]: Plane ID %d arrived at airspace.\n", current_time, current_event->plane_id);
            }

            // Queue the new plane
            enqueue_plane(airport, current_event->plane_id, current_time, log_file);

            // If there's a free runway for landing
            if (airport->busy_runways < airport->total_runways) {
                Plane* landing_plane = dequeue_plane(airport);
                if (landing_plane != NULL) {
                    airport->busy_runways++;
                    total_landed++;

                    printf("   [Runway] Runway available. %s ID %d starts landing process.\n", landing_plane->flight_num, landing_plane->id);
                    if (log_file != NULL) {
                        fprintf(log_file, "   [Runway] Runway available. %s ID %d starts landing process.\n", landing_plane->flight_num, landing_plane->id);
                    }

                    insert_event(airport, EVENT_RUNWAY_FREE, current_time + 10.0, landing_plane->id); // Runway clearance 10 Minutes cooldown

                    free(landing_plane);
                }
            }
            else {
                total_delayed_in_air++;
                printf("   [Runway] All runways busy. Plane ID %d must hover in queue.\n", current_event->plane_id);
                if (log_file != NULL) {
                    fprintf(log_file, "   [Runway] All runways busy. Plane ID %d must hover in queue.\n", current_event->plane_id);
                }
            }
            break;

        case EVENT_RUNWAY_FREE:
            printf("[T = %.2f]: A runway has cleared.\n", current_time);
            if (log_file != NULL) {
                fprintf(log_file, "[T = %.2f]: A runway has cleared.\n", current_time);
            }

            int finished_plane_id = current_event->plane_id;
            printf("   [Terminal] Plane ID %d finished landing. Requesting a gate...\n", finished_plane_id);
            if (log_file != NULL) {
                fprintf(log_file, "   [Terminal] Plane ID %d finished landing. Requesting a gate...\n", finished_plane_id);
            }

            // Call terminal module to dynamically allocate the gate
            if (assign_gate(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols, finished_plane_id)) {
                // Printing the terminal status
                print_terminal_status(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols);

                // 30 Minutes cooldown to free a gate
                insert_event(airport, EVENT_GATE_FREE, current_time + 30.0, finished_plane_id);
            }
            else {
                printf("   [ALERT] Gate allocation failed! Plane ID %d is stuck on taxiway.\n", finished_plane_id);
                if (log_file != NULL) {
                    fprintf(log_file, "   [ALERT] Gate allocation failed! Plane ID %d is stuck on taxiway.\n", finished_plane_id);
                }
            }

            // Free the busy runway
            if (airport->busy_runways > 0) {
                airport->busy_runways--;
            }

            // Check if there are still any planes waiting in the air
            if (airport->queue_head != NULL) {
                Plane* next_plane = dequeue_plane(airport);
                if (next_plane != NULL) {
                    airport->busy_runways++;
                    total_landed++;

                    printf("   [Runway] Next plane cleared to land: %s ID %d.\n", next_plane->flight_num, next_plane->id);
                    if (log_file != NULL) {
                        fprintf(log_file, "   [Runway] Next plane cleared to land: %s ID %d.\n", next_plane->flight_num, next_plane->id);
                    }

                    insert_event(airport, EVENT_RUNWAY_FREE, current_time + 10.0, next_plane->id);
                    free(next_plane);
                }
            }
            else {
                printf("   [Runway] No planes waiting in airspace. Runway remains IDLE.\n");
                if (log_file != NULL) {
                    fprintf(log_file, "   [Runway] No planes waiting in airspace. Runway remains IDLE.\n");
                }
            }
            break;

        case EVENT_GATE_FREE:
            total_departed++;
            printf("[T = %.2f]: Gate Event - Plane ID %d finished terminal operations and is departing.\n", current_time, current_event->plane_id);
            if (log_file != NULL) {
                fprintf(log_file, "[T = %.2f]: Gate Event - Plane ID %d finished terminal operations and is departing.\n", current_time, current_event->plane_id);
            }

            // releasing the gate from the allocated matrix
            release_gate(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols, current_event->plane_id);
            print_terminal_status(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols);
            break;
        }

        free(current_event);
        Sleep(1500);
    }

    printf("--- Simulation Ended at T = %.2f ---\n", current_time);
    if (log_file != NULL) {
        fprintf(log_file, "--- Simulation Ended at T = %.2f ---\n", current_time);
    }

    printf("\n=============================================\n");
    printf("         SIMULATION RESULTS SUMMARY          \n");
    printf("=============================================\n");
    printf(" Total Airplanes Arrived : %d\n", total_arrivals);
    printf(" Total Airplanes Landed  : %d\n", total_landed);
    printf(" Total Airplanes Departed: %d\n", total_departed);
    printf(" Airplanes Delayed in Air: %d\n", total_delayed_in_air);
    printf("=============================================\n");

    if (log_file != NULL) {
        fprintf(log_file, "\n=============================================\n");
        fprintf(log_file, "         SIMULATION RESULTS SUMMARY          \n");
        fprintf(log_file, "=============================================\n");
        fprintf(log_file, " Total Airplanes Arrived : %d\n", total_arrivals);
        fprintf(log_file, " Total Airplanes Landed  : %d\n", total_landed);
        fprintf(log_file, " Total Airplanes Departed: %d\n", total_departed);
        fprintf(log_file, " Airplanes Delayed in Air: %d\n", total_delayed_in_air);
        fprintf(log_file, "=============================================\n");

        fclose(log_file);
    }
}

void enqueue_plane(Airport* airport, int plane_id, double current_time, FILE* log_file) {
    Plane* new_plane = (Plane*)malloc(sizeof(Plane));
    if (new_plane == NULL) {
        fprintf(stderr, "Error: Memory Allocation failed for the new plane.\n");
        return;
    }

    new_plane->id = plane_id;
    strcpy(new_plane->flight_num, "Flight");
    new_plane->arrival_time = current_time;
    new_plane->fuel_level = (rand() % 5) + 1;
    new_plane->state = ARRIVING;
    new_plane->next = NULL;

    printf("   [Queue] %s ID %d enters queue. Fuel level: %d\n", new_plane->flight_num, new_plane->id, new_plane->fuel_level);
    if (log_file != NULL) {
        fprintf(log_file, "   [Queue] %s ID %d enters queue. Fuel level: %d\n", new_plane->flight_num, new_plane->id, new_plane->fuel_level);
    }

    // Emergency State
    if (new_plane->fuel_level == 1) {
        printf("   [EMERGENCY] %s ID %d has low fuel! Priority landing requested.\n", new_plane->flight_num, new_plane->id);
        if (log_file != NULL) {
            fprintf(log_file, "   [EMERGENCY] %s ID %d has low fuel! Priority landing requested.\n", new_plane->flight_num, new_plane->id);
        }

        new_plane->next = airport->queue_head;
        airport->queue_head = new_plane;
        if (airport->queue_tail == NULL) {
            airport->queue_tail = new_plane;
        }
        return;
    }

    // Normal State
    if (airport->queue_tail == NULL) {
        airport->queue_head = new_plane;
        airport->queue_tail = new_plane;
    }
    else {
        airport->queue_tail->next = new_plane;
        airport->queue_tail = new_plane;
    }
}

Plane* dequeue_plane(Airport* airport) {
	if (airport->queue_head == NULL) {
		return NULL;
	}

	Plane* plane_to_remove = airport->queue_head;
	airport->queue_head = airport->queue_head->next;

	if (airport->queue_head == NULL) {
		airport->queue_tail = NULL;
	}

	plane_to_remove->next = NULL;
	return plane_to_remove;
}