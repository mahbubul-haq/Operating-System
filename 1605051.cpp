#include <cstdio>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <queue>
#include <iterator>
#include <random>
using namespace std;

int w, x, y, z, M, N, P, numberOfPassengers, globalTime = 1;

sem_t vip_channel_empty, special_kiosk_empty, boarding_gate_empty;
/// semaphores to block those threads when no passengers to serve

pthread_mutex_t output_mutex, vip_channel_mutex, vip_channel_mutex_rl;

vector<pthread_mutex_t> kiosk_mutex, belt_mutex;
/// kiosk_mutex - if the kiosk is busy
/// belt_mutex - during a belt is serving block other passengers (queue)
vector<sem_t> belt_capacity, belt_empty;
/// belt_capacity - P
/// belt_empty - to block the belt thread when no passenger
vector<queue<int>> belt_passengers; /// passengers in a belt
queue<int> vip_left_right[2];       // 0 queue - left to right, 1 - right to left
queue<int> vip_crossed, special_kiosk_queue, boaring_gate_queue;
/// vip_crossed - temporary while crossing

class Passenger
{
    bool vip;
    int id;

public:
    bool boarding_pass = false;
    Passenger()
    {
        vip = false;
        id = 0;
    }
    Passenger(int id, bool vip = false)
    {
        this->id = id;
        this->vip = vip;
    }
    int getId() { return id; }
    void setId(int id) { this->id = id; }
    bool isVip() { return vip; }
    void makeVIP() { this->vip = true; }
};
vector<Passenger> passengers;

void *journey(void *arg)
{
    int passenger_id = atoi((char *)arg);
    int kioskNumber = (abs(rand()) % M) + 1;

    pthread_mutex_lock(&output_mutex);
    cout << "Passenger " << passenger_id;
    if (passengers[passenger_id - 1].isVip())
        cout << " (VIP)";
    cout << " has arrived at the airport at time " << globalTime << endl;
    pthread_mutex_unlock(&output_mutex);

    pthread_mutex_lock(&kiosk_mutex[kioskNumber - 1]);

    pthread_mutex_lock(&output_mutex);
    cout << "Passenger " << passenger_id;
    if (passengers[passenger_id - 1].isVip())
    {
        cout << " (VIP)";
    }
    cout << " has started self-check in at kiosk " << kioskNumber << " at time " << globalTime << endl;
    pthread_mutex_unlock(&output_mutex);

    sleep(w);
    globalTime += w;

    passengers[passenger_id - 1].boarding_pass = true;

    pthread_mutex_lock(&output_mutex);
    cout << "Passenger " << passenger_id;
    if (passengers[passenger_id - 1].isVip())
        cout << " (VIP)";
    cout << " has finished check in at time " << globalTime << endl;
    pthread_mutex_unlock(&output_mutex);

    pthread_mutex_unlock(&kiosk_mutex[kioskNumber - 1]);

    if (!passengers[passenger_id - 1].isVip())
    {
        int belt_number = abs(rand()) % N;
        pthread_mutex_lock(&output_mutex);
        cout << "Passenger " << passenger_id;
        cout << " has started waiting for security check in belt " << belt_number + 1 << " at " << globalTime << endl;
        pthread_mutex_unlock(&output_mutex);

        sem_wait(&belt_capacity[belt_number]);

        pthread_mutex_lock(&output_mutex);
        cout << "Passenger " << passenger_id;
        cout << " has started the security check at time " << globalTime << endl;

        pthread_mutex_unlock(&output_mutex);

        pthread_mutex_lock(&belt_mutex[belt_number]);
        belt_passengers[belt_number].push(passenger_id);
        sem_post(&belt_empty[belt_number]);
        pthread_mutex_unlock(&belt_mutex[belt_number]);
    }
    else
    {
        pthread_mutex_lock(&output_mutex);
        cout << "Passenger " << passenger_id << " (VIP) has started waiting for the VIP channel at time " << globalTime << endl;
        pthread_mutex_unlock(&output_mutex);

        pthread_mutex_lock(&vip_channel_mutex);
        vip_left_right[0].push(passenger_id);
        sem_post(&vip_channel_empty);

        pthread_mutex_unlock(&vip_channel_mutex);
    }
}

void *belts(void *arg)
{
    int belt_number = atoi((char *)arg);
    belt_number--;

    while (true)
    {
        sem_wait(&belt_empty[belt_number]);
        sem_post(&belt_empty[belt_number]);

        // cout << "wth" << belt_number << endl;

        pthread_mutex_lock(&belt_mutex[belt_number]);

        int cnt = P;
        sleep(x);
        globalTime += x;

        while (cnt > 0 && belt_passengers[belt_number].size() > 0)
        {
            cnt--;
            int passengerId = belt_passengers[belt_number].front();
            belt_passengers[belt_number].pop();

            pthread_mutex_lock(&output_mutex);
            cout << "Passenger " << passengers[passengerId - 1].getId();
            // if (passengers[passengerId - 1].isVip())
            // cout << " (VIP)";
            cout << " has crossed the security check at " << globalTime << endl;
            pthread_mutex_unlock(&output_mutex);
            boaring_gate_queue.push(passengerId);

            pthread_mutex_lock(&output_mutex);
            cout << "Passenger " << passengers[passengerId - 1].getId();
            cout << " has started waiting to be boarded at time " << globalTime << endl;
            pthread_mutex_unlock(&output_mutex);

            sem_post(&boarding_gate_empty);
            sem_post(&belt_capacity[belt_number]);
            sem_wait(&belt_empty[belt_number]);
        }

        pthread_mutex_unlock(&belt_mutex[belt_number]);
    }
}

void *vip_channel_func(void *arg)
{
    while (true)
    {

        sem_wait(&vip_channel_empty);
        sem_post(&vip_channel_empty);

        if (vip_left_right[0].size() > 0)
        {
            pthread_mutex_lock(&vip_channel_mutex);
            while (vip_left_right[0].size() > 0)
            {
                vip_crossed.push(vip_left_right[0].front());
                int passengerId = vip_left_right[0].front();

                pthread_mutex_lock(&output_mutex);
                cout << "Passenger " << passengerId;
                if (passengers[passengerId - 1].isVip())
                    cout << " (VIP)";
                cout << " has got the VIP channel at " << globalTime << endl;
                pthread_mutex_unlock(&output_mutex);

                vip_left_right[0].pop();
            }
            pthread_mutex_unlock(&vip_channel_mutex);

            globalTime += z;
            sleep(z);

            while (vip_crossed.size() > 0)
            {
                int passengerId = vip_crossed.front();
                vip_crossed.pop();
                pthread_mutex_lock(&output_mutex);
                cout << "Passenger " << passengerId;
                if (passengers[passengerId - 1].isVip())
                    cout << " (VIP)";
                cout << " has crossed the VIP channel at time " << globalTime << endl;
                pthread_mutex_unlock(&output_mutex);
                boaring_gate_queue.push(passengerId);

                pthread_mutex_lock(&output_mutex);
                cout << "Passenger " << passengers[passengerId - 1].getId();
                if (passengers[passengerId - 1].isVip())
                    cout << " (VIP)";
                cout << " has started waiting to be boarded at time " << globalTime << endl;
                pthread_mutex_unlock(&output_mutex);

                sem_post(&boarding_gate_empty);
                sem_wait(&vip_channel_empty);
            }
        }
        else if (vip_left_right[1].size() > 0)
        {
            pthread_mutex_lock(&vip_channel_mutex_rl);
            while (vip_left_right[1].size() > 0)
            {
                vip_crossed.push(vip_left_right[1].front());

                int passengerId = vip_left_right[1].front();

                pthread_mutex_lock(&output_mutex);
                cout << "Passenger " << passengerId;
                if (passengers[passengerId - 1].isVip())
                    cout << " (VIP)";
                cout << " has got the VIP channel (right to left) at " << globalTime << endl;
                pthread_mutex_unlock(&output_mutex);

                vip_left_right[1].pop();
            }
            pthread_mutex_unlock(&vip_channel_mutex_rl);

            globalTime += z;
            sleep(z);

            while (vip_crossed.size() > 0)
            {
                int passengerId = vip_crossed.front();
                vip_crossed.pop();
                pthread_mutex_lock(&output_mutex);
                cout << "Passenger " << passengerId;
                if (passengers[passengerId - 1].isVip())
                    cout << " (VIP)";
                cout << " has crossed the VIP channel(right to left) at time " << globalTime << endl;

                pthread_mutex_unlock(&output_mutex);
                special_kiosk_queue.push(passengerId);
                sem_post(&special_kiosk_empty);
                sem_wait(&vip_channel_empty);
            }
        }
    }
}

void *boarding_gate(void *arg)
{
    // cout << "voala" << endl;
    while (true)
    {
        sem_wait(&boarding_gate_empty);
        sem_post(&boarding_gate_empty);

        int passengerId = boaring_gate_queue.front();
        boaring_gate_queue.pop();
        sem_wait(&boarding_gate_empty);
        // cout << passengerId << endl;

        if (abs(rand()) % 4 == 0)
        {
            passengers[passengerId - 1].boarding_pass = false;
        }

        if (passengers[passengerId - 1].boarding_pass)
        {
            pthread_mutex_lock(&output_mutex);
            cout << "Passenger " << passengerId;
            if (passengers[passengerId - 1].isVip())
                cout << " (VIP)";
            cout << " has started boarding on the plane at time " << globalTime << endl;
            pthread_mutex_unlock(&output_mutex);

            sleep(y);
            globalTime += y;

            pthread_mutex_lock(&output_mutex);
            cout << "Passenger " << passengerId;
            if (passengers[passengerId - 1].isVip())
                cout << " (VIP)";
            cout << " has boarded on the plane at time " << globalTime << endl;
            pthread_mutex_unlock(&output_mutex);
        }
        else
        {
            pthread_mutex_lock(&vip_channel_mutex_rl);
            vip_left_right[1].push(passengerId);
            sem_post(&vip_channel_empty);
            pthread_mutex_lock(&output_mutex);
            cout << "Passenger " << passengerId;
            if (passengers[passengerId - 1].isVip())
                cout << " (VIP)";
            cout << " has started waiting for the VIP channel(right side) at time " << globalTime << endl;
            pthread_mutex_unlock(&output_mutex);
            pthread_mutex_unlock(&vip_channel_mutex_rl);
        }
    }
}

void *special_kiosk(void *arg)
{
    while (true)
    {
        sem_wait(&special_kiosk_empty);
        sem_post(&special_kiosk_empty);

        int passengerId = special_kiosk_queue.front();
        special_kiosk_queue.pop();
        sem_wait(&special_kiosk_empty);

        pthread_mutex_lock(&output_mutex);
        cout << "Passenger " << passengerId;
        if (passengers[passengerId - 1].isVip())
        {
            cout << " (VIP)";
        }
        cout << " has started waiting in special kiosk at " << globalTime << endl;
        pthread_mutex_unlock(&output_mutex);
        sleep(w);
        globalTime += w;
        passengers[passengerId - 1].boarding_pass = true;

        pthread_mutex_lock(&output_mutex);
        cout << "Passenger " << passengerId;
        if (passengers[passengerId - 1].isVip())
            cout << " (VIP)";
        cout << " has got his boarding pass from special kiosk at " << globalTime << endl;
        pthread_mutex_unlock(&output_mutex);
        pthread_mutex_lock(&vip_channel_mutex);
        vip_left_right[0].push(passengerId);
        sem_post(&vip_channel_empty);
        pthread_mutex_lock(&output_mutex);
        cout << "Passenger " << passengerId;
        if (passengers[passengerId - 1].isVip())
            cout << " (VIP)";
        cout << " has arrived at VIP channel at time " << globalTime << endl;
        pthread_mutex_unlock(&output_mutex);
        pthread_mutex_unlock(&vip_channel_mutex);
    }
}

int main()
{

    srand(time(NULL));
    freopen("input.txt", "r", stdin);
    // freopen("output.txt", "w", stdout);
    cin >> M >> N >> P;
    cin >> w >> x >> y >> z;

    ///============data generation=============

    pthread_mutex_init(&output_mutex, NULL);
    pthread_mutex_init(&vip_channel_mutex, NULL);
    pthread_mutex_init(&vip_channel_mutex_rl, NULL);
    sem_init(&vip_channel_empty, 0, 0);
    sem_init(&special_kiosk_empty, 0, 0);
    sem_init(&boarding_gate_empty, 0, 0);

    kiosk_mutex.resize(M);
    for (int i = 0; i < M; i++)
    {
        pthread_mutex_init(&kiosk_mutex[i], NULL);
    }

    belt_capacity.assign(N, sem_t());

    belt_passengers.assign(N, queue<int>());

    // pthread_mutex_init(&belt_mutex, NULL);
    belt_empty.assign(N, sem_t());
    belt_mutex.assign(N, pthread_mutex_t());

    for (int i = 0; i < N; i++)
    {
        sem_init(&belt_capacity[i], 0, P);
        sem_init(&belt_empty[i], 0, 0);
        pthread_mutex_init(&belt_mutex[i], NULL);
    }
    pthread_t belt_thread[N];

    for (int i = 0; i < N; i++)
    {
        char *belt_number = new char[10];
        strcpy(belt_number, to_string(i + 1).c_str());
        pthread_create(&belt_thread[i], NULL, belts, (void *)belt_number);
    }

    pthread_t vip_channel_thread;

    pthread_create(&vip_channel_thread, NULL, vip_channel_func, NULL);

    pthread_t boarding_gate_thread;

    pthread_create(&boarding_gate_thread, NULL, boarding_gate, NULL);

    pthread_t special_kiosk_thread;
    pthread_create(&special_kiosk_thread, NULL, special_kiosk, NULL);

    ///===========poisson distribution

    std::random_device rd;  // uniformly-distributed integer random number generator
    std::mt19937 rng(rd()); // mt19937: Pseudo-random number generation

    double averageArrival = 4;
    double lamda = 1 / averageArrival;
    std::exponential_distribution<double> exp(lamda);

    double sumArrivalTimes = 0;
    double newArrivalTime;

    for (int i = 0; i < 15; i++)
    {
        newArrivalTime = exp.operator()(rng); // generates the next random number in the distribution
        sumArrivalTimes = sumArrivalTimes + newArrivalTime;

        sleep((int)newArrivalTime);
        globalTime += (int)newArrivalTime;

        passengers.push_back({i + 1, false});
        if (abs(rand() % 5 == 0))
            passengers[i].makeVIP();

        char *passenger_id = new char[10];
        strcpy(passenger_id, to_string(passengers[i].getId()).c_str());
        pthread_t passenger_thread;

        pthread_create(&passenger_thread, NULL, journey, (void *)passenger_id);
        pthread_join(passenger_thread, NULL);
    }

    for (int i = 0; i < N; i++)
    {
        pthread_join(belt_thread[i], NULL);
    }
    pthread_join(vip_channel_thread, NULL);
    pthread_join(special_kiosk_thread, NULL);
    pthread_join(boarding_gate_thread, NULL);

    // cout << M << endl;
}