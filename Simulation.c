// IMPORTS
#define _CRT_SECURE_NO_WARNINGS
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
    char log_msg[256];
    int total_arrivals = 0;
    int total_landed = 0;
    int total_departed = 0;
    int total_delayed_in_air = 0;

    printf("=== Simulation Started ===\n");
    write_to_log("=== Simulation Started ===");

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
            sprintf(log_msg, "[T = %.2f]: Plane ID %d arrived at airspace.", current_time, current_event->plane_id);
            fprintf(stdout, "%s\n", log_msg);
            write_to_log(log_msg);

            // Queue the new plane
            enqueue_plane(airport, current_event->plane_id, current_time);

            // If there's a free runway for landing
            if (airport->busy_runways < airport->total_runways) {
                Plane* landing_plane = dequeue_plane(airport);
                if (landing_plane != NULL) {
                    airport->busy_runways++;
                    total_landed++;

                    sprintf(log_msg, "   [Runway] Runway available. %s ID %d starts landing process.", landing_plane->flight_num, landing_plane->id);
                    fprintf(stdout, "%s\n", log_msg);
                    write_to_log(log_msg);

                    insert_event(airport, EVENT_RUNWAY_FREE, current_time + 10.0, landing_plane->id); // Runway clearance 10 Minutes cooldown

                    free(landing_plane);
                }
            }
            else {
                total_delayed_in_air++;
                sprintf(log_msg, "   [Runway] All runways busy. Plane ID %d must hover in queue.", current_event->plane_id);
                fprintf(stdout, "%s\n", log_msg);
                write_to_log(log_msg);
            }
            break;

        case EVENT_RUNWAY_FREE:
            sprintf(log_msg, "[T = %.2f]: A runway has cleared.", current_time);
            fprintf(stdout, "%s\n", log_msg);
            write_to_log(log_msg);

            int finished_plane_id = current_event->plane_id;
            sprintf(log_msg, "   [Terminal] Plane ID %d finished landing. Requesting a gate...", finished_plane_id);
            fprintf(stdout, "%s\n", log_msg);
            write_to_log(log_msg);

            // Call terminal module to dynamically allocate the gate
            if (assign_gate(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols, finished_plane_id)) {
                // Printing the terminal status
                print_terminal_status(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols);

                // 30 Minutes cooldown to free a gate
                insert_event(airport, EVENT_GATE_FREE, current_time + 30.0, finished_plane_id);
            }
            else {
                sprintf(log_msg, "   [ALERT] Gate allocation failed! Plane ID %d is stuck on taxiway.", finished_plane_id);
                fprintf(stdout, "%s\n", log_msg);
                write_to_log(log_msg);
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

                    sprintf(log_msg, "   [Runway] Next plane cleared to land: %s ID %d.", next_plane->flight_num, next_plane->id);
                    fprintf(stdout, "%s\n", log_msg);
                    write_to_log(log_msg);

                    insert_event(airport, EVENT_RUNWAY_FREE, current_time + 10.0, next_plane->id);
                    free(next_plane);
                }
            }
            else {
                sprintf(log_msg, "   [Runway] No planes waiting in airspace. Runway remains IDLE.");
                fprintf(stdout, "%s\n", log_msg);
                write_to_log(log_msg);
            }
            break;

        case EVENT_GATE_FREE:
            total_departed++;
            sprintf(log_msg, "[T = %.2f]: Gate Event - Plane ID %d finished terminal operations and is departing.", current_time, current_event->plane_id);
            fprintf(stdout, "%s\n", log_msg);
            write_to_log(log_msg);

            // releasing the gate from the allocated matrix
            release_gate(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols, current_event->plane_id);
            print_terminal_status(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols);
            break;
        }

        free(current_event);
    }
    sprintf(log_msg, "--- Simulation Ended at T = %.2f ---", current_time);
    fprintf(stdout, "%s\n", log_msg);
    write_to_log(log_msg);

    printf("\n=============================================\n");
    printf("         SIMULATION RESULTS SUMMARY          \n");
    printf("=============================================\n");
    printf(" Total Airplanes Arrived : %d\n", total_arrivals);
    printf(" Total Airplanes Landed  : %d\n", total_landed);
    printf(" Total Airplanes Departed: %d\n", total_departed);
    printf(" Airplanes Delayed in Air: %d\n", total_delayed_in_air);
    printf("=============================================\n");

    write_to_log("\n=============================================");
    write_to_log("         SIMULATION RESULTS SUMMARY          ");
    write_to_log("=============================================");
    char stat_line[128];
    sprintf(stat_line, " Total Airplanes Arrived : %d", total_arrivals); write_to_log(stat_line);
    sprintf(stat_line, " Total Airplanes Landed  : %d", total_landed); write_to_log(stat_line);
    sprintf(stat_line, " Total Airplanes Departed: %d", total_departed); write_to_log(stat_line);
    sprintf(stat_line, " Airplanes Delayed in Air: %d", total_delayed_in_air); write_to_log(stat_line);
    write_to_log("=============================================\n");
}

void enqueue_plane(Airport* airport, int plane_id, double current_time) {
    Plane* new_plane = (Plane*)malloc(sizeof(Plane));
    if (new_plane == NULL) {
        fprintf(stderr, "Error: Memory Allocation failed for the new plane.\n");
        return;
    }

    new_plane->id = plane_id;
    strcpy(new_plane->flight_num, "Flight");
    new_plane->arrival_time = current_time;
    new_plane->fuel_level = (rand() % 5) + 1; // Random fuel
    new_plane->state = ARRIVING;
    new_plane->next = NULL;

    char log_msg[256];
    sprintf(log_msg, "   [Queue] %s ID %d enters queue. Fuel level: %d", new_plane->flight_num, new_plane->id, new_plane->fuel_level);
    fprintf(stdout, "%s\n", log_msg);
    write_to_log(log_msg);

    // Emergency State
    if (new_plane->fuel_level == 1) {
        sprintf(log_msg, "   [EMERGENCY] %s ID %d has low fuel! Priority landing requested.", new_plane->flight_num, new_plane->id);
        fprintf(stdout, "%s\n", log_msg);
        write_to_log(log_msg);

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