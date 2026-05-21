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
    double current_time = 0.0;

    printf("=== Simulation Started ===\n");

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
            printf("[T = %.2f]: Plane ID %d arrived at airspace.\n", current_time, current_event->plane_id);

            // Queue the new plane
            enqueue_plane(airport, current_event->plane_id, current_time);

            // If there's a free runway for landing
            if (airport->busy_runways < airport->total_runways) {
                Plane* landing_plane = dequeue_plane(airport);
                if (landing_plane != NULL) {
                    airport->busy_runways++;
                    printf("   [Runway] Runway available. %s ID %d starts landing process.\n",
                        landing_plane->flight_num, landing_plane->id);

                    insert_event(airport, EVENT_RUNWAY_FREE, current_time + 10.0, landing_plane->id); // Runway clearance 10 Minutes cooldown

                    free(landing_plane);
                }
            }
            else {
                printf("   [Runway] All runways busy. Plane ID %d must hover in queue.\n", current_event->plane_id);
            }
            break;

        case EVENT_RUNWAY_FREE:
            printf("[T = %.2f]: A runway has cleared.\n", current_time);

            int finished_plane_id = current_event->plane_id;
            printf("   [Terminal] Plane ID %d finished landing. Requesting a gate...\n", finished_plane_id);

            // Using Omri's function to assign the gate
            if (assign_gate(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols, finished_plane_id)) {
                // Printing the temrinal status
                print_terminal_status(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols);

                // 30 Minutes cooldown to free a gate
                insert_event(airport, EVENT_GATE_FREE, current_time + 30.0, finished_plane_id);
            }
            else {
                printf("   [ALERT] Gate allocation failed! Plane ID %d is stuck on taxiway.\n", finished_plane_id);
            }

            // Free the busy runway
            if (airport->busy_runways > 0) {
                airport->busy_runways--;
            }

            // Check if there are still any planes
            if (airport->queue_head != NULL) {
                Plane* next_plane = dequeue_plane(airport);
                if (next_plane != NULL) {
                    airport->busy_runways++;
                    printf("   [Runway] Next plane cleared to land: %s ID %d.\n",
                        next_plane->flight_num, next_plane->id);

                    insert_event(airport, EVENT_RUNWAY_FREE, current_time + 10.0, next_plane->id);
                    free(next_plane);
                }
            }
            else {
                printf("   [Runway] No planes waiting in airspace. Runway remains IDLE.\n");
            }
            break;

        case EVENT_GATE_FREE:
            printf("[T = %.2f]: Gate Event - Plane ID %d finished terminal operations and is departing.\n",
                current_time, current_event->plane_id);

            // calling to Omri's function to release the gate
            release_gate(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols, current_event->plane_id);

            print_terminal_status(airport->gate_matrix, airport->matrix_rows, airport->matrix_cols);
            break;
        }

        free(current_event);
    }

    printf("--- Simulation Ended at T = %.2f ---\n", current_time);
}

void enqueue_plane(Airport* airport, int plane_id, double current_time) {
	Plane* new_plane = (Plane*)malloc(sizeof(Plane));
	if (new_plane == NULL) {
		fprintf(stderr, "Error: Memory Allocation failed for the new event.\n");
		return;
	}

	strcpy(new_plane->flight_num, "Flight");

	new_plane->id = plane_id;
	new_plane->arrival_time = current_time;
	new_plane->fuel_level = (rand() % 5) + 1;
	new_plane->state = ARRIVING;
	new_plane->next = NULL;


	fprintf(stdout, "   [Queue] %s ID %d enters queue. Fuel level: %d\n",
		new_plane->flight_num, new_plane->id, new_plane->fuel_level); // Random flight number
	new_plane->arrival_time = current_time;
	new_plane->fuel_level = (rand() % 5) + 1; // random fuel level 1-5
	new_plane->state = ARRIVING;
	new_plane->next = NULL;

	fprintf(stdout, "   [Queue] %s ID %d enters queue. Fuel level: %d\n",
		new_plane->flight_num, new_plane->id, new_plane->fuel_level);

	// emergency state
	if (new_plane->fuel_level == 1) {
		fprintf(stdout, "   [EMERGENCY] %s ID %d has low fuel! Priority landing requested.\n",
			new_plane->flight_num, new_plane->id);

		new_plane->next = airport->queue_head;
		airport->queue_head = new_plane;
		if (airport->queue_tail == NULL) {
			airport->queue_tail = new_plane;
		}
		return;
	}

	// normal queue state
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