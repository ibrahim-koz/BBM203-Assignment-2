#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <values.h>
#define _GNU_SOURCE

struct ParsedLine
{
    char **tokens;
    int token_size;
};

struct ParsedLine parse_line(char *line, char *delimiter)
{
    struct ParsedLine parsed_line = {malloc(sizeof(char *)), 0};
    char *token;
    /* get the first token */
    token = strtok(line, delimiter);

    /* walk through other tokens and the tokens*/
    while (token != NULL)
    {
        parsed_line.token_size++;
        parsed_line.tokens = realloc(parsed_line.tokens, sizeof(char *) * parsed_line.token_size);
        parsed_line.tokens[parsed_line.token_size - 1] = malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(parsed_line.tokens[parsed_line.token_size - 1], token);
        token = strtok(NULL, delimiter);
    }
    return parsed_line;
}

void read_and_parse_lines(struct ParsedLine **parsed_lines, int *parsed_lines_size, char *file_name)
{
    //Read the data and parsing it.
    struct ParsedLine parsed_line;
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(file_name, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "") == 0)
            continue;
        (*parsed_lines_size)++;
        *parsed_lines = realloc(*parsed_lines, sizeof(struct ParsedLine) * *parsed_lines_size);
        line[strcspn(line, "\r\n")] = '\0';
        parsed_line = parse_line(line, " ");
        (*parsed_lines)[*(parsed_lines_size)-1] = parsed_line;
    }
    fclose(fp);
    if (line)
        free(line);
}

/*******PARSING IS DONE*******/

struct Passenger
{
    char *passenger_name;
    char *passenger_class;
    char *wanted_seat_class;
    int place;
};

struct PriorityQueue
{
    struct Passenger *waiting_passengers;
    char *que_type;
    int waiting_passenger_number;
};

struct Flight
{
    char *flight_name;

    struct PriorityQueue business_priority_queue;

    struct PriorityQueue economy_priority_queue;

    struct PriorityQueue standard_priority_queue;

    bool is_closed;

    int business_seat_number;
    int economy_seat_number;
    int standard_seat_number;

    int solded_business_seat_number;
    int solded_economy_seat_number;
    int solded_standard_seat_number;

    struct Passenger *business_seats;
    struct Passenger *economy_seats;
    struct Passenger *standard_seats;
};

bool compare_priority_of_child_and_parent(struct Passenger child, struct Passenger parent, char *priority_type)
{
    // return true if child has the priority, likewise return false if parent has the priority.
    bool is_will_be_changed;
    // both of them have no the priority.
    if (strcmp(child.passenger_class, priority_type) != 0 && strcmp(parent.passenger_class, priority_type) != 0)
    {
        if (child.place < parent.place)
            is_will_be_changed = true;
        else
            is_will_be_changed = false;
    }
    // child has the priority but parent hasn't.
    else if (strcmp(child.passenger_class, priority_type) == 0 && strcmp(parent.passenger_class, priority_type) != 0)
        is_will_be_changed = true;
    // both of them have the priority.
    else if (strcmp(child.passenger_class, priority_type) == 0 && strcmp(parent.passenger_class, priority_type) == 0)
    {
        if (child.place < parent.place)
            is_will_be_changed = true;
        else
            is_will_be_changed = false;
    }
    // parent has the priority but child hasn't.
    else if (strcmp(child.passenger_class, priority_type) != 0 && strcmp(parent.passenger_class, priority_type) == 0)
        is_will_be_changed = false;
    return is_will_be_changed;
}

void transform_to_min_priority_que(struct PriorityQueue priority_queue) // building min-heap
{
    char *priority_type;
    if (strcmp(priority_queue.que_type, "business") == 0)
        priority_type = strdup("diplomat");
    else if (strcmp(priority_queue.que_type, "economy") == 0)
        priority_type = strdup("veteran");
    else if (strcmp(priority_queue.que_type, "standard") == 0)
        priority_type = strdup("nobody");

    for (int i = priority_queue.waiting_passenger_number - 1; i > 0; i--)
    { // starting from the end of heap, go through backward.
        struct Passenger *child = &priority_queue.waiting_passengers[i];
        struct Passenger *parent = &priority_queue.waiting_passengers[((i + 1) / 2) - 1];
        if (compare_priority_of_child_and_parent(*child, *parent, priority_type))
        {
            // swap process
            struct Passenger temp = *parent;
            *parent = *child;
            *child = temp;
        }
    }
    free(priority_type);
}

struct Passenger dequeue_highest_priority_from_priority_que(struct PriorityQueue *priority_queue)
{
    struct Passenger highest_priority_passenger = priority_queue->waiting_passengers[0];
    priority_queue->waiting_passengers[0] = priority_queue->waiting_passengers[priority_queue->waiting_passenger_number - 1];
    priority_queue->waiting_passenger_number -= 1;
    priority_queue->waiting_passengers = realloc(priority_queue->waiting_passengers, sizeof(struct Passenger) * (priority_queue->waiting_passenger_number));
    return highest_priority_passenger;
}

void sell_ticket(struct Passenger **seats, int *solded_seat_number, struct Passenger passenger)
{
    (*seats)[*(solded_seat_number)] = passenger;
    (*(solded_seat_number))++;
}

int search_flight(char *flight_name, struct Flight *flights, int flight_num)
{
    int index_of_flight = -1;
    for (int i = 0; i < flight_num; i++)
        if (strcmp(flights[i].flight_name, flight_name) == 0)
        {
            index_of_flight = i;
            break;
        }
    return index_of_flight;
}

void add_new_seats(struct Flight *flight, char *que_type, int number_of_will_be_added_seats)
{
    if (strcmp(que_type, "business") == 0)
    {
        flight->business_seat_number += number_of_will_be_added_seats;
        flight->business_seats = realloc(flight->business_seats, sizeof(struct Passenger) * flight->business_seat_number);
    }
    else if (strcmp(que_type, "economy") == 0)
    {
        flight->economy_seat_number += number_of_will_be_added_seats;
        flight->economy_seats = realloc(flight->economy_seats, sizeof(struct Passenger) * flight->economy_seat_number);
    }
    else if (strcmp(que_type, "standard") == 0)
    {
        flight->standard_seat_number += number_of_will_be_added_seats;
        flight->standard_seats = realloc(flight->standard_seats, sizeof(struct Passenger) * flight->standard_seat_number);
    }
}

struct PriorityQueue create_priority_queue(char *que_type)
{
    struct PriorityQueue new_priority_queue = {malloc(sizeof(struct Passenger)), strdup(que_type), 0};
    return new_priority_queue;
}

struct Flight init_new_flight(char *flight_name)
{
    struct Flight new_flight;
    new_flight.flight_name = strdup(flight_name);

    new_flight.is_closed = false;

    new_flight.business_seat_number = 0;
    new_flight.economy_seat_number = 0;
    new_flight.standard_seat_number = 0;

    new_flight.solded_business_seat_number = 0;
    new_flight.solded_economy_seat_number = 0;
    new_flight.solded_standard_seat_number = 0;

    new_flight.business_seats = malloc(sizeof(struct Passenger));
    new_flight.economy_seats = malloc(sizeof(struct Passenger));
    new_flight.standard_seats = malloc(sizeof(struct Passenger));

    new_flight.business_priority_queue = create_priority_queue("business");
    new_flight.economy_priority_queue = create_priority_queue("economy");
    new_flight.standard_priority_queue = create_priority_queue("standard");

    return new_flight;
}

struct Passenger form_new_passenger(char *passenger_name, char *passenger_class)
{
    struct Passenger new_passenger;
    new_passenger.passenger_name = strdup(passenger_name);
    new_passenger.passenger_class = strdup(passenger_class);
    return new_passenger;
}

void realloc_priority_que(struct PriorityQueue *priority_queue)
{ //editlenecek 2x hale getirilecek
    priority_queue->waiting_passenger_number++;
    priority_queue->waiting_passengers = realloc(priority_queue->waiting_passengers, sizeof(struct Passenger) * priority_queue->waiting_passenger_number);
}

void priority_que_enqueue(struct PriorityQueue *priority_queue, struct Passenger *new_passenger)
{
    realloc_priority_que(priority_queue);
    new_passenger->place = priority_queue->waiting_passenger_number;
    priority_queue->waiting_passengers[priority_queue->waiting_passenger_number - 1] = *new_passenger;
}

int how_many_seat_can_be_sold(int available_seat_number, int demand_number)
{
    int number_of_seat_will_be_sold;
    if (available_seat_number >= demand_number)
        number_of_seat_will_be_sold = demand_number;
    else
        number_of_seat_will_be_sold = available_seat_number;
    return number_of_seat_will_be_sold;
}

bool is_sufficient_number_of_seat(int seat_number, int demand_number)
{
    bool is_sufficient;
    if (seat_number >= demand_number)
        is_sufficient = true;
    else
        is_sufficient = false;
    return is_sufficient;
}

void free_passenger(struct Passenger *passenger)
{
    free(passenger->passenger_name);
    free(passenger->passenger_class);
    free(passenger->wanted_seat_class);
}

void free_priority_queue(struct PriorityQueue *priority_queue)
{
    for (int i = 0; i < priority_queue->waiting_passenger_number; i++)
        free_passenger(&(priority_queue->waiting_passengers[i]));
    free(priority_queue->waiting_passengers);
    free(priority_queue->que_type);
}

int main(int argc, char *argv[])
{

    struct ParsedLine *parsed_lines = malloc(sizeof(struct ParsedLine));
    int parsed_lines_size = 0;

    read_and_parse_lines(&parsed_lines, &parsed_lines_size, argv[1]);

    struct Flight *flights = malloc(sizeof(struct Flight));
    int flight_num = 0;

    // Running commands according to the parsed lines
    FILE *fp;
    fp = fopen(argv[2], "w");

    for (int i = 0; i < parsed_lines_size; i++)
    {
        if (strcmp(parsed_lines[i].tokens[0], "addseat") == 0)
        {
            int index_of_flight = search_flight(parsed_lines[i].tokens[1], flights, flight_num);
            if (index_of_flight == -1)
            {
                struct Flight new_flight = init_new_flight(parsed_lines[i].tokens[1]);
                add_new_seats(&new_flight, parsed_lines[i].tokens[2], atoi(parsed_lines[i].tokens[3]));
                flight_num++;
                flights = realloc(flights, sizeof(struct Flight) * flight_num);
                flights[flight_num - 1] = new_flight;
                fprintf(fp, "%s %s %d %d %d\n", "addseats", new_flight.flight_name, new_flight.business_seat_number,
                        new_flight.economy_seat_number, new_flight.standard_seat_number);
            }
            else if (index_of_flight != -1 && !flights[index_of_flight].is_closed)
            {
                add_new_seats(&(flights[index_of_flight]), parsed_lines[i].tokens[2], atoi(parsed_lines[i].tokens[3]));
                fprintf(fp, "%s %s %d %d %d\n", "addseats", flights[index_of_flight].flight_name, flights[index_of_flight].business_seat_number,
                        flights[index_of_flight].economy_seat_number, flights[index_of_flight].standard_seat_number);
            }
        }

        else if (strcmp(parsed_lines[i].tokens[0], "enqueue") == 0)
        {
            int index_of_flight = search_flight(parsed_lines[i].tokens[1], flights, flight_num);
            if (index_of_flight != -1 && !flights[index_of_flight].is_closed)
            {
                struct Passenger new_passenger;
                if (parsed_lines[i].token_size == 5)
                    new_passenger = form_new_passenger(parsed_lines[i].tokens[3], parsed_lines[i].tokens[4]);
                else
                    new_passenger = form_new_passenger(parsed_lines[i].tokens[3], "normal");

                if (strcmp(parsed_lines[i].tokens[2], "business") == 0)
                {
                    if (parsed_lines[i].token_size == 4 || (parsed_lines[i].token_size == 5 && strcmp(parsed_lines[i].tokens[4], "diplomat") == 0))
                    {
                        new_passenger.wanted_seat_class = strdup("business");
                        priority_que_enqueue(&(flights[index_of_flight].business_priority_queue), &new_passenger);
                        fprintf(fp, "%s %s %s %s %d\n", "queue", flights[index_of_flight].flight_name, new_passenger.passenger_name,
                                "business", flights[index_of_flight].business_priority_queue.waiting_passenger_number);
                    }
                    else
                    {
                        free(new_passenger.passenger_name);
                        free(new_passenger.passenger_class);
                        fprintf(fp, "error\n");
                    }
                }

                else if (strcmp(parsed_lines[i].tokens[2], "economy") == 0)
                {

                    if (parsed_lines[i].token_size == 4 || (parsed_lines[i].token_size == 5 && strcmp(parsed_lines[i].tokens[4], "veteran") == 0))
                    {
                        new_passenger.wanted_seat_class = strdup("economy");
                        priority_que_enqueue(&(flights[index_of_flight].economy_priority_queue), &new_passenger);
                        fprintf(fp, "%s %s %s %s %d\n", "queue", flights[index_of_flight].flight_name, new_passenger.passenger_name,
                                "economy", flights[index_of_flight].economy_priority_queue.waiting_passenger_number);
                    }
                    else
                    {
                        free(new_passenger.passenger_name);
                        free(new_passenger.passenger_class);
                        fprintf(fp, "error\n");
                    }
                }
                else if (strcmp(parsed_lines[i].tokens[2], "standard") == 0)
                {
                    if (parsed_lines[i].token_size == 4 || (parsed_lines[i].token_size == 5 && !(strcmp(parsed_lines[i].tokens[4], "diplomat") == 0 || strcmp(parsed_lines[i].tokens[4], "veteran") == 0)))
                    {
                        new_passenger.wanted_seat_class = strdup("standard");
                        priority_que_enqueue(&(flights[index_of_flight].standard_priority_queue), &new_passenger);
                        fprintf(fp, "%s %s %s %s %d\n", "queue", flights[index_of_flight].flight_name, new_passenger.passenger_name,
                                "standard", flights[index_of_flight].standard_priority_queue.waiting_passenger_number);
                    }
                    else
                    {
                        free(new_passenger.passenger_name);
                        free(new_passenger.passenger_class);
                        fprintf(fp, "error\n");
                    }
                }

                else
                    fprintf(fp, "error\n");
            }
        }

        else if (strcmp(parsed_lines[i].tokens[0], "sell") == 0)
        {
            int index_of_flight = search_flight(parsed_lines[i].tokens[1], flights, flight_num);
            if (index_of_flight != -1 && !flights[index_of_flight].is_closed)
            {
                int t = how_many_seat_can_be_sold(flights[index_of_flight].business_seat_number - flights[index_of_flight].solded_business_seat_number,
                                                  flights[index_of_flight].business_priority_queue.waiting_passenger_number);

                for (int i = 0; i < t; i++)
                {
                    transform_to_min_priority_que(flights[index_of_flight].business_priority_queue);
                    struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].business_priority_queue));
                    sell_ticket(&(flights[index_of_flight].business_seats), &(flights[index_of_flight].solded_business_seat_number), highest_priority_passenger);
                }

                t = how_many_seat_can_be_sold(flights[index_of_flight].economy_seat_number - flights[index_of_flight].solded_economy_seat_number,
                                              flights[index_of_flight].economy_priority_queue.waiting_passenger_number);
                for (int i = 0; i < t; i++)
                {
                    transform_to_min_priority_que(flights[index_of_flight].economy_priority_queue);
                    struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].economy_priority_queue));
                    sell_ticket(&(flights[index_of_flight].economy_seats), &(flights[index_of_flight].solded_economy_seat_number), highest_priority_passenger);
                }

                t = how_many_seat_can_be_sold(flights[index_of_flight].standard_seat_number - flights[index_of_flight].solded_standard_seat_number,
                                              flights[index_of_flight].standard_priority_queue.waiting_passenger_number);
                for (int i = 0; i < t; i++)
                {
                    transform_to_min_priority_que(flights[index_of_flight].standard_priority_queue);
                    struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].standard_priority_queue));
                    sell_ticket(&(flights[index_of_flight].standard_seats), &(flights[index_of_flight].solded_standard_seat_number), highest_priority_passenger);
                }

                if (flights[index_of_flight].business_seat_number - flights[index_of_flight].solded_business_seat_number == 0)
                {
                    t = how_many_seat_can_be_sold(flights[index_of_flight].standard_seat_number - flights[index_of_flight].solded_standard_seat_number,
                                                  flights[index_of_flight].business_priority_queue.waiting_passenger_number);

                    for (int i = 0; i < t; i++)
                    {
                        transform_to_min_priority_que(flights[index_of_flight].business_priority_queue);
                        struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].business_priority_queue));
                        priority_que_enqueue(&(flights[index_of_flight].standard_priority_queue), &highest_priority_passenger);
                    }
                }

                t = how_many_seat_can_be_sold(flights[index_of_flight].standard_seat_number - flights[index_of_flight].solded_standard_seat_number,
                                              flights[index_of_flight].standard_priority_queue.waiting_passenger_number);
                for (int i = 0; i < t; i++)
                {
                    transform_to_min_priority_que(flights[index_of_flight].standard_priority_queue);
                    struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].standard_priority_queue));
                    sell_ticket(&(flights[index_of_flight].standard_seats), &(flights[index_of_flight].solded_standard_seat_number), highest_priority_passenger);
                }

                if (flights[index_of_flight].economy_seat_number - flights[index_of_flight].solded_economy_seat_number == 0)
                {
                    t = how_many_seat_can_be_sold(flights[index_of_flight].standard_seat_number - flights[index_of_flight].solded_standard_seat_number,
                                                  flights[index_of_flight].economy_priority_queue.waiting_passenger_number);
                    for (int i = 0; i < t; i++)
                    {
                        transform_to_min_priority_que(flights[index_of_flight].economy_priority_queue);
                        struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].economy_priority_queue));
                        priority_que_enqueue(&(flights[index_of_flight].standard_priority_queue), &highest_priority_passenger);
                    }
                }

                t = how_many_seat_can_be_sold(flights[index_of_flight].standard_seat_number - flights[index_of_flight].solded_standard_seat_number,
                                              flights[index_of_flight].standard_priority_queue.waiting_passenger_number);
                for (int i = 0; i < t; i++)
                {
                    transform_to_min_priority_que(flights[index_of_flight].standard_priority_queue);
                    struct Passenger highest_priority_passenger = dequeue_highest_priority_from_priority_que(&(flights[index_of_flight].standard_priority_queue));
                    sell_ticket(&(flights[index_of_flight].standard_seats), &(flights[index_of_flight].solded_standard_seat_number), highest_priority_passenger);
                }

                fprintf(fp, "%s %s %d %d %d\n", "sold", flights[index_of_flight].flight_name, flights[index_of_flight].solded_business_seat_number,
                        flights[index_of_flight].solded_economy_seat_number, flights[index_of_flight].solded_standard_seat_number);
            }

            else
                fprintf(fp, "error\n");
        }

        else if (strcmp(parsed_lines[i].tokens[0], "close") == 0)
        {
            int index_of_flight = search_flight(parsed_lines[i].tokens[1], flights, flight_num);
            if (index_of_flight != -1)
            {
                flights[index_of_flight].is_closed = true;
                int total_ticket_number = flights[index_of_flight].solded_business_seat_number + flights[index_of_flight].solded_economy_seat_number + flights[index_of_flight].solded_standard_seat_number;
                int total_waiting_passenger_number = flights[index_of_flight].business_priority_queue.waiting_passenger_number + flights[index_of_flight].economy_priority_queue.waiting_passenger_number + flights[index_of_flight].standard_priority_queue.waiting_passenger_number;
                fprintf(fp, "%s %s %d %d\n", "closed", flights[index_of_flight].flight_name, total_ticket_number, total_waiting_passenger_number);
                for (int i = 0; i < flights[index_of_flight].business_priority_queue.waiting_passenger_number; i++)
                    fprintf(fp, "%s %s\n", "waiting", flights[index_of_flight].business_priority_queue.waiting_passengers[i].passenger_name);
                for (int i = 0; i < flights[index_of_flight].economy_priority_queue.waiting_passenger_number; i++)
                    fprintf(fp, "%s %s\n", "waiting", flights[index_of_flight].economy_priority_queue.waiting_passengers[i].passenger_name);
                for (int i = 0; i < flights[index_of_flight].standard_priority_queue.waiting_passenger_number; i++)
                    fprintf(fp, "%s %s\n", "waiting", flights[index_of_flight].standard_priority_queue.waiting_passengers[i].passenger_name);
            }
        }

        else if (strcmp(parsed_lines[i].tokens[0], "report") == 0)
        {
            int index_of_flight = search_flight(parsed_lines[i].tokens[1], flights, flight_num);
            if (index_of_flight != -1)
            {
                fprintf(fp, "%s %s\n", "report", flights[index_of_flight].flight_name);

                fprintf(fp, "%s %d\n", "business", flights[index_of_flight].solded_business_seat_number);
                for (int i = 0; i < flights[index_of_flight].solded_business_seat_number; i++)
                    fprintf(fp, "%s\n", flights[index_of_flight].business_seats[i].passenger_name);

                fprintf(fp, "%s %d\n", "economy", flights[index_of_flight].solded_economy_seat_number);
                for (int i = 0; i < flights[index_of_flight].solded_economy_seat_number; i++)
                    fprintf(fp, "%s\n", flights[index_of_flight].economy_seats[i].passenger_name);

                fprintf(fp, "%s %d\n", "standard", flights[index_of_flight].solded_standard_seat_number);
                for (int i = 0; i < flights[index_of_flight].solded_standard_seat_number; i++)
                    fprintf(fp, "%s\n", flights[index_of_flight].standard_seats[i].passenger_name);

                fprintf(fp, "%s %s\n", "end of report", flights[index_of_flight].flight_name);
            }
        }

        else if (strcmp(parsed_lines[i].tokens[0], "info") == 0)
        {
            bool is_will_be_break = false;
            char *passenger_name = strdup(parsed_lines[i].tokens[1]);
            for (int i = 0; i < flight_num; i++)
            {
                for (int j = 0; j < flights[i].solded_business_seat_number; j++)
                {
                    if (strcmp(flights[i].business_seats[j].passenger_name, passenger_name) == 0)
                    {
                        fprintf(fp, "%s %s %s %s %s\n", "info", flights[i].business_seats[j].passenger_name, flights[i].flight_name, flights[i].business_seats[j].wanted_seat_class, "business");
                        is_will_be_break = true;
                        break;
                    }
                }

                if (is_will_be_break)
                    break;

                for (int j = 0; j < flights[i].solded_economy_seat_number; j++)
                {
                    if (strcmp(flights[i].economy_seats[j].passenger_name, passenger_name) == 0)
                    {

                        fprintf(fp, "%s %s %s %s %s\n", "info", flights[i].economy_seats[j].passenger_name, flights[i].flight_name, flights[i].economy_seats[j].wanted_seat_class, "economy");
                        is_will_be_break = true;
                        break;
                    }
                }

                if (is_will_be_break)
                    break;

                for (int j = 0; j < flights[i].solded_standard_seat_number; j++)
                {
                    if (strcmp(flights[i].standard_seats[j].passenger_name, passenger_name) == 0)
                    {

                        fprintf(fp, "%s %s %s %s %s\n", "info", flights[i].standard_seats[j].passenger_name, flights[i].flight_name, flights[i].standard_seats[j].wanted_seat_class, "standard");
                        is_will_be_break = true;
                        break;
                    }
                }

                if (is_will_be_break)
                    break;

                for (int j = 0; j < flights[i].business_priority_queue.waiting_passenger_number; j++)
                {
                    if (strcmp(flights[i].business_priority_queue.waiting_passengers[j].passenger_name, passenger_name) == 0)
                    {
                        fprintf(fp, "%s %s %s %s %s\n", "info", flights[i].business_priority_queue.waiting_passengers[j].passenger_name, flights[i].flight_name, flights[i].business_priority_queue.waiting_passengers[j].wanted_seat_class, "none");
                        is_will_be_break = true;
                        break;
                    }
                }

                if (is_will_be_break)
                    break;

                for (int j = 0; j < flights[i].economy_priority_queue.waiting_passenger_number; j++)
                {
                    if (strcmp(flights[i].economy_priority_queue.waiting_passengers[j].passenger_name, passenger_name) == 0)
                    {
                        fprintf(fp, "%s %s %s %s %s\n", "info", flights[i].economy_priority_queue.waiting_passengers[j].passenger_name, flights[i].flight_name, flights[i].economy_priority_queue.waiting_passengers[j].wanted_seat_class, "none");
                        is_will_be_break = true;
                        break;
                    }
                }

                if (is_will_be_break)
                    break;

                for (int j = 0; j < flights[i].standard_priority_queue.waiting_passenger_number; j++)
                {
                    if (strcmp(flights[i].standard_priority_queue.waiting_passengers[j].passenger_name, passenger_name) == 0)
                    {
                        fprintf(fp, "%s %s %s %s %s\n", "info", flights[i].standard_priority_queue.waiting_passengers[j].passenger_name, flights[i].flight_name, flights[i].standard_priority_queue.waiting_passengers[j].wanted_seat_class, "none");
                        is_will_be_break = true;
                        break;
                    }
                }
                if (is_will_be_break)
                    break;
            }
            if (!is_will_be_break)
                fprintf(fp, "error\n");
            free(passenger_name);
        }
    }

    fclose(fp);

    //free

    for (int i = 0; i < flight_num; i++)
    {
        free(flights[i].flight_name);
        free_priority_queue(&(flights[i].business_priority_queue));
        free_priority_queue(&(flights[i].economy_priority_queue));
        free_priority_queue(&(flights[i].standard_priority_queue));

        for (int j = 0; j < flights[i].solded_business_seat_number; j++)
        {
            free_passenger(&(flights[i].business_seats[j]));
        }

        for (int j = 0; j < flights[i].solded_economy_seat_number; j++)
        {
            free_passenger(&(flights[i].economy_seats[j]));
        }

        for (int j = 0; j < flights[i].solded_standard_seat_number; j++)
        {
            free_passenger(&(flights[i].standard_seats[j]));
        }

        free(flights[i].business_seats);
        free(flights[i].economy_seats);
        free(flights[i].standard_seats);
    }

    free(flights);

    for (int i = 0; i < parsed_lines_size; i++)
    {
        for (int j = 0; j < parsed_lines[i].token_size; j++)
            free(parsed_lines[i].tokens[j]);
        free(parsed_lines[i].tokens);
    }

    free(parsed_lines);

    return 0;
}
