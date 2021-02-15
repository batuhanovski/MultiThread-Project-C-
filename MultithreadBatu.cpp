#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fstream>
#include <semaphore.h>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <thread>

using namespace std;


int num_clients;         // number of clients
int num_seats;           // number of seats which can vary according to the theater's name
bool seatCondition[300]; // boolean array which has true for empty seats, and false for fulls. 0 index is not used.


/* thread functions */
void *teller(void *param);
void *client(void *param);

sem_t freeTellers;      // semaphore which shows the number of free tellers, used for synchronization
sem_t readyClients;     // semaphore which shows the number of ready clients, used for synchronization

/* mutexes */
pthread_mutex_t mutexA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexAt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBt= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadmutex = PTHREAD_MUTEX_INITIALIZER;

/* client names initialized to "null" */
string clientNameA="null";
string clientNameB="null";
string clientNameC="null";

/* global buffer variables specified for every teller */
int desiredSeatA;
int desiredSeatB;
int desiredSeatC;
int tellerSleepA;
int tellerSleepB;
int tellerSleepC;
int capacity;

/* global boolean variables which shows the availability of each teller */
bool isAfree;
bool isBfree;
bool isCfree;

int num_servicedClients=0;   // global variable which shows the number of serviced clients
int iseveryoneServiced=0;    // global booelan variable which remarks whether everyone is serviced or not

ofstream out_file;

/**
 * author: Batuhan Tongarlakk
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {

    std::ifstream file(argv[1]);   // input file taken from shell
    out_file.open(argv[2]);
    std::string theater;
    file >> theater;                  // the name of the theater specified in the input
    file >> num_clients;              // number of the clients specified in the input

    std::string clientInfos[num_clients];  // string which have all the lines of client stats in the input


    for (int i = 0; i < num_clients; ++i) { // filling the clientInfos array with reading from input
        file >> clientInfos[i];             //
    }                                       //

    if(theater=="OdaTiyatrosu"){                // num_seat variable will be initialized the proper capacity with
        num_seats=60;}                          // respect to theater's names
    if(theater=="UskudarStudyoSahne"){          //
        num_seats=80;}                          //
    if(theater=="KucukSahne"){                  //
        num_seats=200;}                         //
    capacity=num_seats;                         //

    for (bool & i : seatCondition) {            // initializing all the indexes in the seatCondition array to true,
        i=true;    }                            // because ever seat is free at the start

    for (int i = 1+capacity; i <300; ++i) {     // after filling the capacity with true, the rest of the array will be
        seatCondition[i]= false;        }       // useless, so they are initialized with false


    out_file << "Welcome to the Sync-Ticket!" << endl;

    isAfree = true; // All tellers are free at the start.
    isBfree = true; //
    isCfree = true; //

    pthread_attr_t attr;        // thread attribution set up
    pthread_attr_init(&attr);   //

    pthread_t tid[3];           // teller threads
    pthread_t cid[num_clients]; // clients threads

    sem_init(&freeTellers,0,3);  // initializing semaphores, 3 to freeTellers, 0 to readyClients.
    sem_init(&readyClients,0,0); //

    char tellerKinds[3] = {'A','B','C'};        // teller kinds, A, B or C

    for(int i = 0; i < 3; i++){                                         //
        pthread_mutex_lock(&threadmutex);                               // creation of teller threads.
        pthread_create(&tid[i], &attr, &teller, &tellerKinds[i]);   }   //

    for (int i = 0; i < num_clients; ++i) {                             // creation of client threads.
        pthread_create(&cid[i],&attr,&client, &clientInfos[i]);   }     //

    for(int i = 0; i < num_clients; i++) {                          //  join function for clients.
        pthread_join(cid[i], NULL);    }                //

    for (int i = 0; i < 3; ++i) {                                   // join function for tellers.
        pthread_join(tid[i],NULL);    }


    return 0;
}
/**
 * client function for client threads
 * first, clients sleep until their waiting time has finished,
 * after when there is available teller, the one (head of the queue goes to the teller)
 */

/**
 * client function for client threads
 * first, clients sleep until their waiting time has finished,
 * after when there is available teller, the one (head of the queue goes to the teller)
 * @param param the string which has all the data of the specific client
 */
void *client(void *param) {
        string info = *(string *)param; // info is the whole string coming from main, this string has client name,
                                        // waiting time, process time and desired seat of each client
        string name;                    // name of the client
        int waitTime;                   // wait time of the client
        int processTime;                // the time needed while teller doing his/her work for client
        int desiredSeat;                // desired seat number of the client


        /* some string operations to initialize name, waitTime, processTime and desiredSeat */
        stringstream ss(info);

    for (int i = 0; i < 4; ++i) {
        string substr;
        getline(ss, substr, ',');
        if(i==0) {
            name = substr;
        }
        if(i==1){
            waitTime = stoi(substr);
        }
        if(i==2){
            processTime = stoi(substr);
        }
        if (i==3){
            desiredSeat=stoi(substr);
        }
    }

        usleep(1000*waitTime);  // first, client waits her/his wait time with usleep function


        sem_wait(&freeTellers);       // semaphore allows a client pass if freetellers are not 0. It was initially 3.

        /* The teller A has priority among B and C, and B has priority among C. This order has
         * provided with some sequential if statements. First clients goes in to the part A if teller A
         * is free, else it goes to B and so on.
         * Example:
         * If teller i is not doing anything, the next client will get into this part.
         * In that part, the global and specific variables of teller i is filled with
         * this specific client's infos.
         * After this stage,the semaphore readyClients is incremented by 1. (It will be used in teller function) */
        if(isAfree){
            pthread_mutex_lock(&mutexA);
            isAfree=false;
            clientNameA=name;
            tellerSleepA=processTime;
            desiredSeatA=desiredSeat;
            sem_post(&readyClients);
            pthread_mutex_unlock(&mutexA);
            pthread_exit(NULL);
        }

        else if (isBfree){

            pthread_mutex_lock(&mutexA);
            isBfree=false;
            clientNameB=name;
            tellerSleepB=processTime;
            desiredSeatB=desiredSeat;
            sem_post(&readyClients);
            pthread_mutex_unlock(&mutexA);

            pthread_exit(NULL);
        }

        else if(isCfree){

            pthread_mutex_lock(&mutexA);
            isCfree=false;
            clientNameC=name;
            tellerSleepC=processTime;
            desiredSeatC=desiredSeat;
            sem_post(&readyClients);
            pthread_mutex_unlock(&mutexA);
            pthread_exit(NULL);
        }

        pthread_exit(NULL);
}
/**
 * teller function for teller threads, there are also 3 parts for teller A, B and C.
 * @param param 'A', 'B' or 'C' which shows the tellers name.
 */
void *teller(void *param) {

    char kind = *(char *) param;

    int givenseatA;
    int givenseatB;
    int givenseatC;
    int check = 2;

    pthread_mutex_lock(&mutex);
    out_file << "Teller " << kind << " has arrived." << endl;
    pthread_mutex_unlock(&threadmutex);
    pthread_mutex_unlock(&mutex);

    /* Every teller is in an infinite loop inside this while loop. There are three parts for teller A, B and C.
     * If teller A's global variable clientName is not "null" (as I initialized before everything), teller A does his(her work.
     * Same for B and C tellers, but if there are additional conditions.
     * Inside every if statement, ready clients semaphore is decremented by 1. After, the seat arrangements are done, if the desired seat is full or
     * it is in progress, the teller will give the available seat wich has minimum seat number, and if there is no such a seat, it won't give a seat.
     * Tellers sleeps (does the ticket works) with respect to the clients process time. After, sleeping, he/she gives ticket to the client. Then the teller
     * changes the corresponding tellers free condition, and changes his/her gloabal variable name = "null".
     * If everyone is serviced, the last if statements captures threads and in that part they will exit.
     */

    while (1) {
        if (kind == 'A' && clientNameA != "null") {
            sem_wait(&readyClients);
            if (seatCondition[desiredSeatA]) {
                givenseatA = desiredSeatA;
                pthread_mutex_lock(&mutexAt);
                seatCondition[desiredSeatA] = false;
                pthread_mutex_unlock(&mutexAt);
            } else {
                check = 0;
                for (int i = 1; i <= capacity; ++i) {
                    if (seatCondition[i]) {
                        givenseatA = i;
                        check = 1;
                        pthread_mutex_lock(&mutexAt);
                        seatCondition[i] = false;
                        pthread_mutex_unlock(&mutexAt);
                        break;
                    }
                }
            }

            usleep(1000 * tellerSleepA);
            pthread_mutex_lock(&mutex);
            if (check != 0) {
                out_file << clientNameA << " requests seat " << desiredSeatA << ", reserves seat " << givenseatA
                     << ". Signed by Teller A." << endl;
            }  else{
                out_file << clientNameA << " requests seat " << desiredSeatA << ", reserves None. Signed by Teller A." << endl;
            }
            isAfree = true;
            pthread_mutex_unlock(&mutex);

            pthread_mutex_lock(&mutexAt);
            num_servicedClients++;

            clientNameA = "null";
            sem_post(&freeTellers);
            pthread_mutex_unlock(&mutexAt);


        } else if (kind == 'B' && clientNameB != "null" && !isAfree) {
            sem_wait(&readyClients);
            if (seatCondition[desiredSeatB]) {
                givenseatB = desiredSeatB;
                pthread_mutex_lock(&mutexBt);
                seatCondition[desiredSeatB] = false;
                pthread_mutex_unlock(&mutexBt);
            } else {
                check = 0;
                for (int i = 1; i <= capacity; ++i) {
                    if (seatCondition[i]) {
                        givenseatB = i;
                        check = 1;
                        pthread_mutex_lock(&mutexBt);
                        seatCondition[i] = false;
                        pthread_mutex_unlock(&mutexBt);
                        break;
                    }
                }
            }


            usleep(1000 * tellerSleepB);
            pthread_mutex_lock(&mutex);
            if (check != 0) {
                out_file << clientNameB << " requests seat " << desiredSeatB << ", reserves seat " << givenseatB
                     << ". Signed by Teller B." << endl;
            }  else{
                out_file << clientNameB << " requests seat " << desiredSeatB << ", reserves seat None. Signed by Teller B." << endl;
            }
            isBfree = true;
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&mutexBt);
            num_servicedClients++;

            clientNameB = "null";
            sem_post(&freeTellers);
            pthread_mutex_unlock(&mutexBt);


        } else if (kind == 'C' && clientNameC != "null" && !isAfree && !isBfree) {
            sem_wait(&readyClients);
            if (seatCondition[desiredSeatC]) {
                givenseatC = desiredSeatC;
                pthread_mutex_lock(&mutexCt);
                seatCondition[desiredSeatC] = false;
                pthread_mutex_unlock(&mutexCt);
            } else {
                check = 0;
                for (int i = 1; i <= capacity; ++i) {
                    if (seatCondition[i]) {
                        givenseatC = i;
                        check = 1;
                        pthread_mutex_lock(&mutexCt);
                        seatCondition[i] = false;
                        pthread_mutex_unlock(&mutexCt);
                        break;
                    }
                }
            }

            usleep(1000 * tellerSleepC);

            pthread_mutex_lock(&mutex);
            if (check != 0) {
                out_file << clientNameC << " requests seat " << desiredSeatC << ", reserves seat " << givenseatC
                     << ". Signed by Teller C." << endl;
            }
            else{
                out_file << clientNameC << " requests seat " << desiredSeatC << ", reserves seat None. Signed by Teller C." << endl;
            }
            isCfree = true;
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&mutexCt);
            num_servicedClients++;

            clientNameC = "null";
            sem_post(&freeTellers);
            pthread_mutex_unlock(&mutexCt);


        }
        if (num_servicedClients == num_clients) {
            pthread_mutex_lock(&mutex);
            iseveryoneServiced = 1;
            pthread_mutex_unlock(&mutex);
        }

        if (iseveryoneServiced) {
            if (kind == 'A') {
                out_file << "All clients received service." << endl;
                pthread_exit(NULL);
            } else {
                pthread_exit(NULL);
            }

        }
    }
}
