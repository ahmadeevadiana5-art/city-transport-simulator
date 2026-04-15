#pragma once
#include "Transport.h"
#include <map>
#include <random>

class TransportSimulation {
protected:
    Graph* busGraph;
    Graph* tramGraph;
    Graph* compositeGraph;
    vector<Route*> routes;
    vector<Vehicle*> vehicles;
    vector<vector<Passenger*>> stopsWaiting;

    int currentTime;
    mt19937 rng;

public:
    TransportSimulation(Graph* busG, Graph* tramG, Graph* compositeG)
        : busGraph(busG), tramGraph(tramG), compositeGraph(compositeG),
        currentTime(0), rng(random_device{}()) {
        stopsWaiting.resize(compositeGraph->N);
    }

    ~TransportSimulation() {
        for (Route* r : routes) delete r;
        for (Vehicle* v : vehicles) 
        {
            // Удаляем пассажиров внутри транспорта
                for (Passenger* p : v->passengersInside) 
                {
                    delete p;
                }
            v->passengersInside.clear();
            delete v;
        }

        for (auto& stop : stopsWaiting) {
            for (Passenger* p : stop) delete p;
        }
    }

    void addRoute(Route* route) {
        routes.push_back(route);
    }

    void addVehicle(Vehicle* vehicle) {
        vehicles.push_back(vehicle);
    }

    void tick() {
        currentTime++;

        for (Vehicle* v : vehicles) {
            if (v->minutesToNextStop > 0) {
                v->minutesToNextStop--;  
            }


            // Приехали на остановку
            if (v->minutesToNextStop == 0) {
                // 1. ВЫСАДКА ПАССАЖИРОВ
                vector<Passenger*> stillInside;
                int exitedCount = 0;

                for (Passenger* p : v->passengersInside) {
                    if (p->destinationStop == v->currentStopVertex) {
                        // Пассажир приехал — выходит
                        delete p;
                        exitedCount++;
                    }
                    else {
                        // Едет дальше
                        stillInside.push_back(p);
                    }
                }

                v->passengersInside = stillInside;
                v->passengers = v->passengersInside.size();  // Обновляем счётчик


                /*
            if (v->minutesToNextStop == 0) {*/
            // 2. Посадка новых пассажиров
                boardPassengers(v);       
                //3. едем дальше 

                v->moveToNextStop();   
            }
        }

        if (currentTime % 10 == 0) generatePassengers();
        if (currentTime % 30 == 0) printStatus();
    }

    void run(int minutes) {
        for (int m = 0; m < minutes; ++m) {
            tick();
        }
    }

private:
    void boardPassengers(Vehicle* v) {
        int stopVertex = v->currentStopVertex;
        auto& waiting = stopsWaiting[stopVertex];

        if (waiting.empty()) return;

        vector<Passenger*> willBoard;
        vector<Passenger*> willWait;

        /*
        for (Passenger* p : waiting) {
            bool canReach = false;

            if (v->route->isBus()) {
                canReach = v->route->servesStop(p->destinationStop);
            }
            else {
                canReach = v->route->servesStop(p->destinationStop);
            }

            if (canReach) {
                willBoard.push_back(p);
            }
            else {
                willWait.push_back(p);
            }
        }*/

        // Разделяем: кто садится, кто ждёт
        for (Passenger* p : waiting) {
            if (v->route->servesStop(p->destinationStop)) {
                willBoard.push_back(p);
            }
            else {
                willWait.push_back(p);
            }
        }

        int freeSeats = v->capacity - v->passengers;
        int boardCount = min((int)willBoard.size(), freeSeats);

        /*
        for (int i = 0; i < boardCount; ++i) {
            v->passengers++;
            delete willBoard[i];
        }
        */

        // Сажаем пассажиров
        for (int i = 0; i < boardCount; ++i) {
            v->passengersInside.push_back(willBoard[i]);  // ← Добавляем в список внутри

        }
        // Обновляем счётчик пассажиров
        v->passengers = v->passengersInside.size();


        // Кто не влез — остаётся ждать
        for (int i = boardCount; i < willBoard.size(); ++i) {
            willWait.push_back(willBoard[i]);
        }

        // Кто не подходит по маршруту — тоже ждёт
        stopsWaiting[stopVertex] = willWait;
    }

    /*
    //БЕЗ увеличения пассажиров в часы пик
    void generatePassengers() {
        if (!compositeGraph || compositeGraph->N == 0) return;

        uniform_int_distribution<int> stopDist(0, compositeGraph->N - 1);
        uniform_int_distribution<int> countDist(0, 10);

        int numStopsToGenerate = 1 + (rng() % 3);

        for (int i = 0; i < numStopsToGenerate; i++) {
            int stop = stopDist(rng);
            int count = countDist(rng) / 2;

            for (int j = 0; j < count; j++) {
                int dest = stopDist(rng);
                while (dest == stop) dest = stopDist(rng);
                stopsWaiting[stop].push_back(new Passenger(dest));
            }
        }
    }
    */

    //Увеличили число пассжиров в часы пик
    void generatePassengers() {
        if (!compositeGraph || compositeGraph->N == 0) return;

        uniform_int_distribution<int> stopDist(0, compositeGraph->N - 1);

        // Определяем коэффициент в зависимости от времени суток
        int totalMinutes = currentTime;
        int startHour = 4;
        int absoluteMinutes = startHour * 60 + totalMinutes;
        int hour = (absoluteMinutes / 60) % 24;

        int maxPassengers;
        int numStopsToGenerate;

        if ((hour >= 7 && hour < 9) || (hour >= 17 && hour < 19)) {
            // ЧАС ПИК — много людей на многих остановках
            maxPassengers = 35;
            numStopsToGenerate = 5 + (rng() % 4);  // 4-7 остановок
        }
        else if (hour >= 23 || hour < 4) {
            // НОЧЬ — почти никого
            maxPassengers = 5;
            numStopsToGenerate = 1 + (rng() % 2);  // 1-2 остановки
        }
        else if (hour >= 4 && hour < 7) {
            // РАННЕЕ УТРО — мало людей
            maxPassengers = 10;
            numStopsToGenerate = 2 + (rng() % 2);  // 2-3 остановки
        }
        else if (hour >= 9 && hour < 17) {
            // ДЕНЬ — средний поток
            maxPassengers = 18;
            numStopsToGenerate = 3 + (rng() % 3);  // 3-5 остановок
        }
        else {
            // ВЕЧЕР (19-23) — умеренный поток
            maxPassengers = 15;
            numStopsToGenerate = 2 + (rng() % 3);  // 2-4 остановки
        }

        uniform_int_distribution<int> countDist(0, maxPassengers);

        for (int i = 0; i < numStopsToGenerate; i++) {
            int stop = stopDist(rng);
            int count = countDist(rng) / 2;  // Делим на 2 для реалистичности (не все сразу приходят)

            for (int j = 0; j < count; j++) {
                int dest = stopDist(rng);
                while (dest == stop) dest = stopDist(rng);
                stopsWaiting[stop].push_back(new Passenger(dest));
            }
        }
    }

    void printStatus() {
        cout << "\n=== Time " << (currentTime / 60) << ":"
            << setw(2) << setfill('0') << (currentTime % 60) << " ===\n";

        for (Vehicle* v : vehicles) {
            cout << v->id << ": stop." << v->currentStopVertex
                << " pass." << v->passengers << "/" << v->capacity;
            if (v->minutesToNextStop > 0) {
                cout << " (moving " << v->minutesToNextStop << " min)";
            }
            cout << "\n";
        }
    }
};