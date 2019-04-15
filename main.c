#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>


struct station
{
    pthread_mutex_t mutex;              //Mutual Exclusion variable to lock and unlock the critical section
    pthread_cond_t train_arrived;       //This condition must be known by the passenger to know that it is allowed to move on board the train
    pthread_cond_t passenger_is_seated;    //This condition must be known by the train to know when it can move from the station
    int available_seats;
    int number_of_passengers;
    int waiting_passengers;
};

void station_init(struct station *station)             //Initialize the station objects when the system boots
{
    pthread_mutex_init(&station->mutex, NULL);
    pthread_cond_init(&station->train_arrived, NULL);
    pthread_cond_init(&station->passenger_is_seated, NULL);
    station->waiting_passengers = 0;

    printf("\t\t\t Welcome to train automation\n\n");
    printf("Please specify number of passengers:\t");
    scanf("%d", &station->number_of_passengers);

    printf("\nPlease specify number of seats in the train:\t");
    scanf("%d", &station->available_seats);

    if(station->number_of_passengers > station->available_seats)                //To separate between passengers who will be seated and the remained ones who will not have places left
    {
        station->waiting_passengers = station->number_of_passengers - station->available_seats;
        station->number_of_passengers = station->number_of_passengers - station->waiting_passengers;
    }
}

void station_load_train(struct station *station)
{
    //Start critical section
    pthread_mutex_lock(&station->mutex);

    pthread_cond_broadcast(&station->train_arrived);

    printf("\nTrain has just arrived in the station and has opened its doors\n");

    /*Train waits in the station till all passengers are in their seats and either the train is full or all waiting passengers have boarded*/
    if(station->number_of_passengers==0)
    {
        printf("\nThere is no passengers!\nThe train will leave the station\n");
        exit(station_load_train);
    }
    else
    {
        while(station->available_seats > 0 && station->number_of_passengers!=0)
            pthread_cond_wait(&station->passenger_is_seated, &station->mutex);

        if(station->available_seats==0 || station->number_of_passengers==0 || station->waiting_passengers==0)
        {
            printf("\nThe train is full and has left the station\n");
            exit(station_load_train);
        }
    }
    //End critical section
    pthread_mutex_unlock(&station->mutex);
}

void station_on_board(struct station *station)
{
    //Start critical section
    pthread_mutex_lock(&station->mutex);

    pthread_cond_signal(&station->passenger_is_seated);

    while(station->number_of_passengers > 0 && station->available_seats > 0)
    {
        station->number_of_passengers--;
        station->available_seats--;
    }
    while(station->waiting_passengers > 0)
    {
        printf("\nPlease specify number of passengers:\t");
        scanf("%d", &station->number_of_passengers);
        printf("\nPlease specify number of seats in the train:\t");
        scanf("%d", &station->available_seats);

        station->waiting_passengers = station->waiting_passengers + station->number_of_passengers;          //New comers + waiting ones
        pthread_t train;
        pthread_create(&train, NULL, station_load_train, station);              //A new train must come since the previous one is full

        while(station->waiting_passengers > 0 && station->available_seats > 0)
        {
            station->waiting_passengers--;
            station->available_seats--;
        }
    }
    //End critical section
    pthread_mutex_unlock(&station->mutex);
}

void station_wait_for_train(struct station *station)
{
    //Start critical section
    pthread_mutex_lock(&station->mutex);

    /*Passenger waits until a train is in the station and there are enough free seats on the train*/
    while(station->number_of_passengers!=0 && station->available_seats!=0)
    {
        pthread_cond_wait(&station->train_arrived, &station->mutex);
        //End critical section
        pthread_mutex_unlock(&station->mutex);
        station_on_board(station);
    }
}

int main()
{
    struct station station;
    station_init(&station);

    int i;
    pthread_t passengers;
    pthread_t train;

    pthread_create(&train, NULL, station_load_train, &station);

    for( i=0; i<station.number_of_passengers; i++ )             //Creating threads for each passenger
            pthread_create(&passengers, NULL, station_wait_for_train, &station);

    pthread_exit(NULL);

    return 0;
}
