#include "Simulation.h"
#include <conio.h>
#include<thread>
#include<chrono>

#include <limits>
#include <windows.h>
#undef max
#include<fstream>
#include<sstream>
#include<ctime>


using namespace std;

string logFilename;
string csvFilename;

string getTimestamp()
{
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);

    stringstream ss;
    ss << 1900 + ltm.tm_year
        << setw(2) << setfill('0') << 1 + ltm.tm_mon
        << setw(2) << setfill('0') << ltm.tm_mday << "_"
        << setw(2) << setfill('0') << ltm.tm_hour
        << setw(2) << setfill('0') << ltm.tm_min
        << setw(2) << setfill('0') << ltm.tm_sec;
    return ss.str();
}

//переменные для управления симуляцией 
bool paused = false;
bool stepMode = false;
int displayFilter = 0; // 0 - все, 1 - автобусы, 2 - трамваи
int selectedStop = -1;
string selectedVehicle = "";
bool showOccupancyPercent = true;

// Функция очистки экрана
void clearScreen() {
    system("cls");
}

/*
// Определение времени суток и коэффициента замедления
double getTimeOfDayFactor(int totalMinutes) 
{
    int hour = (totalMinutes / 60) % 24;
    int minute = totalMinutes % 60;

    // Транспорт не работает с 23:00 до 4:00
    if (hour >= 23 || hour < 4) return 0.0;

    // Часы пик: 7:00-9:00 и 17:00-19:00
    if ((hour >= 7 && hour < 9) || (hour >= 17 && hour < 19)) 
    {
        return 1.5; // Замедление на 50%
    }
    return 1.0;
}

string getTimeOfDayName(int totalMinutes) 
{
    int startHour = 4;
    int absoluteMinutes = startHour * 60 + totalMinutes;
    int hour = (absoluteMinutes / 60) % 24;

    if (hour >= 23 || hour < 4) return "Night (no service)";
    if (hour >= 7 && hour < 9) return "Morning Rush Hour (50% slowdown)";


    if (hour >= 17 && hour < 19) return "Evening Rush Hour (50% slowdown)";
    if (hour >= 4 && hour < 7) return "Early Morning";
    if (hour >= 9 && hour < 17) return "Daytime";
    return "Evening";
}
*/






double getTimeOfDayFactor(int totalMinutes) {
    int startHour = 4;
    int absoluteMinutes = startHour * 60 + totalMinutes;
    int hour = (absoluteMinutes / 60) % 24;

    if (hour >= 23 || hour < 4) return 0.0;
    if ((hour >= 7 && hour < 9) || (hour >= 17 && hour < 19)) return 1.5;
    return 1.0;
}

string getTimeOfDayName(int totalMinutes) {
    int startHour = 4;
    int absoluteMinutes = startHour * 60 + totalMinutes;
    int hour = (absoluteMinutes / 60) % 24;

    if (hour >= 23 || hour < 4) return "Night (no service)";
    if (hour >= 7 && hour < 9) return "Morning Rush Hour (50% slowdown)";
    if (hour >= 17 && hour < 19) return "Evening Rush Hour (50% slowdown)";
    if (hour >= 4 && hour < 7) return "Early Morning";
    if (hour >= 9 && hour < 17) return "Daytime";
    return "Evening";
}

class InteractiveSimulation : public TransportSimulation {
public:
    InteractiveSimulation(Graph* busG, Graph* tramG, Graph* compositeG)
        : TransportSimulation(busG, tramG, compositeG) {}

    void showMenu() {
        clearScreen();
        cout << "\n";
        cout << "==========================================================\n";
        cout << "            CITY TRANSPORT SIMULATOR v2.0                 \n";
        cout << "==========================================================\n";
        cout << "  Current time: " << formatTime(currentTime) << "                          \n";
        cout << "  Mode: " << getTimeOfDayName(currentTime) << "     \n";
        cout << "==========================================================\n";
        cout << "                                                          \n";
        cout << "  SIMULATION CONTROL:                                  \n";
        cout << "  [P] - Pause / Resume                                \n";
        cout << "  [S] - Step mode (1 step = 1 minute)                \n";
        cout << "  [R] - Run continuously                   \n";
        cout << "                                                          \n";
        cout << "  DISPLAY FILTERS:                                    \n";
        cout << "  [1] - Show ALL vehicles                \n";
        cout << "  [2] - Show BUSES only                                    \n";
        cout << "  [3] - Show TRAMS only                                    \n";
        cout << "  [4] - Stop information                            \n";
        cout << "  [5] - Vehicle information                         \n";
        cout << "                                                          \n";
        cout << "  STATISTICS:                                             \n";
        cout << "  [T] - Show general statistics                         \n";
        cout << "  [O] - Toggle occupancy percentage                   \n";
        cout << "                                                          \n";
        cout << "  [Q] - Quit program                                \n";
        cout << "==========================================================\n";
        cout << "\n  Your choice: ";
    }

    string formatTime(int minutes) {
        int hour = 4 + (minutes / 60);
        int min = minutes % 60;
        hour = hour % 24;
        stringstream ss;
        ss << setw(2) << setfill('0') << hour << ":"
            << setw(2) << setfill('0') << min;
        return ss.str();
    }

    void showVehicles() {
        cout << "\n=== " << formatTime(currentTime) << " ["
            << getTimeOfDayName(currentTime) << "] ===\n\n";

        int shown = 0;
        for (Vehicle* v : vehicles) {
            // Применяем фильтр
            if (displayFilter == 1 && v->route->type != BUS) continue;
            if (displayFilter == 2 && v->route->type != TRAM) continue;

            shown++;
            cout << v->id << ": ";
            cout << "stop " << v->currentStopVertex << " | ";
            cout << "pass. " << v->passengers << "/" << v->capacity;

            /*
            cout << left << setw(5) << v->id << ": ";
            cout << "stop" << setw(2) << v->currentStopVertex << " | ";
            cout << "pass." << setw(3) << v->passengers << "/"
                << setw(3) << v->capacity;*/

            if (showOccupancyPercent) {
                int percent = (v->passengers * 100) / v->capacity;
                cout << " ("  << percent << "%)";
            }

            if (v->minutesToNextStop > 0) {
                cout << " | moving " <<  v->minutesToNextStop << " min";
            }
            else {
                cout << " | at stop";
            }

            // Цветовая индикация загруженности
            if (v->passengers > v->capacity * 0.8) {
                cout << " [Overloaded!]";
            }
            else if (v->passengers > v->capacity * 0.5) {
                cout << " [medium load]";
            }

            cout << "\n";
        }

        if (shown == 0) {
            cout << "No vehicles to display.\n";
        }
    }



    void showStopInfo(int stopId) {
        if (stopId < 0 || stopId >= stopsWaiting.size()) {
            cout << "Error: invalid stop number.\n";
            return;
        }

        clearScreen();
        cout << "\n========================================================\n";
        cout << "               STOP INFORMATION" << setw(2) << stopId << "                \n";
        cout << "\n========================================================\n\n";

        size_t waiting = stopsWaiting[stopId].size();
        cout << "Waiting passengers: " << waiting << "\n\n";

        cout << "Vehicles at stop:\n";
        bool found = false;
        for (Vehicle* v : vehicles) {
            if (v->currentStopVertex == stopId) {
                found = true;
                cout << "  * " << v->id << " ("
                    << (v->route->type == BUS ? "Bus" : "Tram") << ")\n";
                cout << "    Passengers: " << v->passengers << "/" << v->capacity;
                cout << " (" << (v->passengers * 100 / v->capacity) << "%)\n";
            }
        }

        if (!found) {
            cout << "  No vehicles at this stop.\n";
        }

        cout << "\nPress any key to continue...";
        _getch();
    }

    void showVehicleInfo(const string& vehicleId) {
        clearScreen();
        cout << "\n========================================================\n";
        cout << "          VEHICLE INFORMATION               \n";
        cout << "==========================================================\n\n";

        for (Vehicle* v : vehicles) {
            if (v->id == vehicleId) {
                cout << "ID: " << v->id << "\n";
                cout << "Type: " << (v->route->type == BUS ? "Bus" : "Tram") << "\n";
                cout << "Route: " << v->route->number << "\n";
                cout << "Capacity: " << v->capacity << "\n";
                cout << "Current stop: " << v->currentStopVertex << "\n";
                cout << "Passengers: " << v->passengers << "/" << v->capacity;
                cout << " (" << (v->passengers * 100 / v->capacity) << "%)\n";
                cout << "Status: ";
                if (v->minutesToNextStop > 0) {
                    cout << "moving (" << v->minutesToNextStop << " min to next)\n";
                }
                else {
                    cout << "at stop\n";
                }

                cout << "\nНPress any key to continue...";
                _getch();
                return;
            }
        }

        cout << "Vehicle with ID  '" << vehicleId << "' not found.\n";
        cout << "\nPress any key to continue...";
        _getch();
    }

    void showStatistics() {
        clearScreen();
        cout << "\n========================================================\n";
        cout << "                    GENERAL STATISTICS                      \n";
        cout << "==========================================================\n\n";

        int totalPassengers = 0;
        int totalCapacity = 0;
        int busCount = 0, tramCount = 0;
        int busPassengers = 0, tramPassengers = 0;
        int busCapacity = 0, tramCapacity = 0;

        for (Vehicle* v : vehicles) {
            totalPassengers += v->passengers;
            totalCapacity += v->capacity;

            if (v->route->type == BUS) {
                busCount++;
                busPassengers += v->passengers;
                busCapacity += v->capacity;
            }
            else {
                tramCount++;
                tramPassengers += v->passengers;
                tramCapacity += v->capacity;
            }
        }

        cout << "Simulation time: " << formatTime(currentTime) << "\n";
        cout << "Mode: " << getTimeOfDayName(currentTime) << "\n\n";

        cout << "TOTAL VEHICLES: " << vehicles.size() << "\n";
        cout << "  Buses: " << busCount << "\n";
        cout << "  Trams: " << tramCount << "\n\n";

        cout << "OVERALL OCCUPANCY:\n";
        cout << "  Total passengers: " << totalPassengers << " / " << totalCapacity;
        cout << " (" << (totalPassengers * 100 / totalCapacity) << "%)\n\n";

        cout << "BUSES:\n";
        cout << "  Passengers: " << busPassengers << " / " << busCapacity;
        cout << " (" << (busPassengers * 100 / busCapacity) << "%)\n\n";

        cout << "TRAMS:\n";
        cout << "  Passengers: " << tramPassengers << " / " << tramCapacity;
        cout << " (" << (tramPassengers * 100 / tramCapacity) << "%)\n\n";

        cout << "WAITING AT STOPS:\n";
        for (int i = 0; i < stopsWaiting.size(); i++) {
            if (!stopsWaiting[i].empty()) {
                cout << "  Stop  " << setw(2) << i << ": "
                    << stopsWaiting[i].size() << " people.\n";
            }
        }

        cout << "\nPress any key to continue...";
        _getch();
    }


    void writeToLog() {
        ofstream log(logFilename, ios::app);
        double factor = getTimeOfDayFactor(currentTime);

        if (factor > 0) {
            log << "\n=== " << formatTime(currentTime) << " ["
                << getTimeOfDayName(currentTime) << "] ===\n";
            for (Vehicle* v : vehicles) {
                log << v->id << ": stop " << v->currentStopVertex
                    << " | pass. " << v->passengers << "/" << v->capacity << "\n";
            }
        }
        else
        {
            log << "\n=== " << formatTime(currentTime) << " [" << getTimeOfDayName(currentTime) << "] ===\n";
        }
        

       
        log.close();
    }

    void writeToCSV() {
        bool exists = ifstream(csvFilename).good();
        ofstream csv(csvFilename, ios::app);
        if (!exists) {
            csv << "Time,Vehicle,Type,Stop,Passengers,Capacity\n";
        }
        for (Vehicle* v : vehicles) {
            csv << formatTime(currentTime) << ","
                << v->id << ","
                << (v->route->type == BUS ? "Bus" : "Tram") << ","
                << v->currentStopVertex << ","
                << v->passengers << ","
                << v->capacity << "\n";
        }
        csv.close();
    }


    
    /*void runInteractive(int totalMinutes) {
        clearScreen();
        showMenu();

        for (int m = 0; m < totalMinutes; ++m) {
            // Проверяем, работает ли транспорт в это время
            if (getTimeOfDayFactor(currentTime) > 0) 
            {
                // Здесь логика симуляции (вызов tick())
                // Упрощенно - вызываем tick() родительского класса
            }

            // Обработка ввода пользователя
            if (_kbhit()) {
                char key = _getch();
                key = toupper(key);

                switch (key) {
                case 'P':
                    paused = !paused;
                    break;
                case 'S':
                    stepMode = true;
                    paused = false;
                    break;
                case 'R':
                    stepMode = false;
                    paused = false;
                    break;
                case '1':
                    displayFilter = 0;
                    break;
                case '2':
                    displayFilter = 1;
                    break;
                case '3':
                    displayFilter = 2;
                    break;
                case '4':
                    cout << "\nEnter stop number (0-9):: ";
                    cin >> selectedStop;
                    showStopInfo(selectedStop);
                    break;
                case '5':
                    cout << "\nEnter vehicle ID (e.g., A101): ";
                    cin >> selectedVehicle;
                    showVehicleInfo(selectedVehicle);
                    break;
                case 'T':
                    showStatistics();
                    break;
                case 'O':
                    showOccupancyPercent = !showOccupancyPercent;
                    break;
                case 'Q':
                    cout << "\nExiting program...\n";
                    return;
                }

                clearScreen();
                showMenu();
            }

            // Если не на паузе - выполняем шаг симуляции
            if (!paused) {
                double factor = getTimeOfDayFactor(currentTime);
                if (factor > 0) {
                    tick();
                    showVehicles();
                }
                else {
                    cout << "\n=== " << formatTime(currentTime) << " [Night break] ===\n";
                    cout << "No transport service from 23:00 to 4:00\n";
                }
                //
                tick();
                //showVehicles();
                

                if (stepMode) {
                    paused = true;
                }

                // Небольшая задержка для читаемости
                this_thread::sleep_for(chrono::milliseconds(100));
            }

            currentTime++;
        }
    }*/

    void runInteractive(int totalHours) {
        clearScreen();
        showMenu();
        showVehicles();

        writeToLog();
        writeToCSV();

        int hour = 0;
        while (hour < totalHours) {
            cout << "\n============================================================\n";
            cout << "  Current hour: " << formatTime(currentTime) << " (" << hour + 1 << "/" << totalHours << ")\n";
            cout << "  [ENTER] - Next hour\n";
            cout << "  [T] - Statistics  [4] - Stop info  [5] - Vehicle info\n";
            cout << "  [1] - All  [2] - Buses  [3] - Trams  [Q] - Quit\n";
            cout << "============================================================\n";
            cout << "  Your choice: ";

            string input;
            getline(cin, input);

            if (input.empty()) {
                // ========== ENTER - СЛЕДУЮЩИЙ ЧАС ==========
                double factor = getTimeOfDayFactor(currentTime);

                if (factor > 0) {
                    for (int min = 0; min < 60; ++min) {
                        tick();
                    }
                }
                else {
                    currentTime += 60;
                }

                hour++;

                clearScreen();
                showMenu();

                if (factor > 0) {
                    showVehicles();
                }
                else {
                    cout << "\n=== " << formatTime(currentTime) << " [Night break] ===\n";
                    cout << "Transport is not running.\n";
                }

                writeToLog();
                writeToCSV();
            }
            else {
                // ========== ДРУГИЕ КОМАНДЫ ==========
                char key = toupper(input[0]);

                switch (key) {
                case '1':
                    displayFilter = 0;
                    break;
                case '2':
                    displayFilter = 1;
                    break;
                case '3':
                    displayFilter = 2;
                    break;
                case '4':
                    cout << "Enter stop number (0-9): ";
                    cin >> selectedStop;
                    cin.ignore(1000, '\n');
                    showStopInfo(selectedStop);
                    cout << "\nPress ENTER to return to menu...";
                    cin.ignore(1000, '\n');
                    break;
                case '5':
                    cout << "Enter vehicle ID (e.g., A101): ";
                    cin >> selectedVehicle;
                    cin.ignore(1000, '\n');
                    showVehicleInfo(selectedVehicle);
                    cout << "\nPress ENTER to return to menu...";
                    cin.ignore(1000, '\n');
                    break;
                case 'T':
                    showStatistics();
                    cout << "\nPress ENTER to return to menu...";
                    cin.ignore(1000, '\n');
                    break;
                case 'O':
                    showOccupancyPercent = !showOccupancyPercent;
                    break;
                case 'Q':
                    cout << "\nExiting program...\n";
                    cout << "\nPress any key to exit...";
                    _getch();
                    return;
                }

                // После любой команды (кроме Q) перерисовываем меню
                clearScreen();
                showMenu();
                showVehicles();
            }
        }

        cout << "\n============================================================\n";
        cout << "  SIMULATION COMPLETE! 24 hours finished.\n";
        cout << "============================================================\n";
        cout << "\nPress any key to exit...";
        _getch();
    }

};






int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    string timestamp = getTimestamp();
    logFilename = "simulation_log_" + timestamp + ".txt";
    csvFilename = "simulation_data_" + timestamp + ".csv";
    cout << "Files will be saved:\n";
    cout << "  " << logFilename << "\n";
    cout << "  " << csvFilename << "\n\n";

    
    // 10 остановок (0-9)
    Graph* busNetwork = new Graph(10, true);
    Graph* tramNetwork = new Graph(10, true);
    Graph* cityMap = new Graph(10, true);

    // ========== АВТОБУСНАЯ СЕТЬ ==========
    busNetwork->adjMatrix[0][1] = 4; busNetwork->adjMatrix[1][0] = 4;
    busNetwork->adjMatrix[1][2] = 3; busNetwork->adjMatrix[2][1] = 3;
    busNetwork->adjMatrix[2][3] = 5; busNetwork->adjMatrix[3][2] = 5;
    busNetwork->adjMatrix[3][4] = 4; busNetwork->adjMatrix[4][3] = 4;
    busNetwork->adjMatrix[2][5] = 7; busNetwork->adjMatrix[5][2] = 7;
    busNetwork->adjMatrix[5][6] = 3; busNetwork->adjMatrix[6][5] = 3;
    busNetwork->adjMatrix[6][7] = 4; busNetwork->adjMatrix[7][6] = 4;
    busNetwork->adjMatrix[4][8] = 6; busNetwork->adjMatrix[8][4] = 6;
    busNetwork->adjMatrix[8][9] = 2; busNetwork->adjMatrix[9][8] = 2;
    busNetwork->adjMatrix[9][0] = 5; busNetwork->adjMatrix[0][9] = 5;

    // ========== ТРАМВАЙНАЯ СЕТЬ ==========
    tramNetwork->adjMatrix[0][2] = 5; tramNetwork->adjMatrix[2][0] = 5;
    tramNetwork->adjMatrix[2][4] = 4; tramNetwork->adjMatrix[4][2] = 4;
    tramNetwork->adjMatrix[4][6] = 6; tramNetwork->adjMatrix[6][4] = 6;
    tramNetwork->adjMatrix[6][8] = 3; tramNetwork->adjMatrix[8][6] = 3;
    tramNetwork->adjMatrix[1][3] = 4; tramNetwork->adjMatrix[3][1] = 4;
    tramNetwork->adjMatrix[3][5] = 5; tramNetwork->adjMatrix[5][3] = 5;
    tramNetwork->adjMatrix[5][7] = 4; tramNetwork->adjMatrix[7][5] = 4;
    tramNetwork->adjMatrix[7][9] = 5; tramNetwork->adjMatrix[9][7] = 5;

    // ========== ОБЩИЙ ГРАФ ==========
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (busNetwork->adjMatrix[i][j] != 0)
                cityMap->adjMatrix[i][j] = busNetwork->adjMatrix[i][j];
            if (tramNetwork->adjMatrix[i][j] != 0)
                cityMap->adjMatrix[i][j] = tramNetwork->adjMatrix[i][j];
        }
    }

    InteractiveSimulation sim(busNetwork, tramNetwork, cityMap);

    // Создание маршрутов и транспорта (оставь как было)
    Route* bus1 = new Route("1", BUS);
    bus1->addStop(0, 0);
    bus1->addStop(1, 4);
    bus1->addStop(2, 3);
    bus1->addStop(3, 5);
    bus1->addStop(4, 4);
    bus1->addStop(0, 5);
    sim.addRoute(bus1);

    Route* bus2 = new Route("2", BUS);
    bus2->addStop(2, 0);
    bus2->addStop(5, 7);
    bus2->addStop(6, 3);
    bus2->addStop(7, 4);
    bus2->addStop(2, 7);
    sim.addRoute(bus2);

    Route* bus3 = new Route("3", BUS);
    bus3->addStop(4, 0);
    bus3->addStop(8, 6);
    bus3->addStop(9, 2);
    bus3->addStop(0, 5);
    bus3->addStop(4, 4);
    sim.addRoute(bus3);

    Route* bus4 = new Route("4", BUS);
    bus4->addStop(1, 0);
    bus4->addStop(2, 3);
    bus4->addStop(5, 7);
    bus4->addStop(2, 7);
    sim.addRoute(bus4);

    Route* tram1 = new Route("11", TRAM);
    tram1->addStop(0, 0);
    tram1->addStop(2, 5);
    tram1->addStop(4, 4);
    tram1->addStop(6, 6);
    tram1->addStop(8, 3);
    tram1->addStop(0, 5);
    sim.addRoute(tram1);

    Route* tram2 = new Route("12", TRAM);
    tram2->addStop(1, 0);
    tram2->addStop(3, 4);
    tram2->addStop(5, 5);
    tram2->addStop(7, 4);
    tram2->addStop(9, 5);
    tram2->addStop(1, 5);
    sim.addRoute(tram2);

    Route* tram3 = new Route("13", TRAM);
    tram3->addStop(3, 0);
    tram3->addStop(5, 5);
    tram3->addStop(7, 4);
    tram3->addStop(9, 5);
    tram3->addStop(8, 4);
    tram3->addStop(6, 3);
    tram3->addStop(4, 6);
    tram3->addStop(2, 4);
    tram3->addStop(0, 5);
    tram3->addStop(3, 5);
    sim.addRoute(tram3);

    sim.addVehicle(new Vehicle("A101", 50, bus1));
    sim.addVehicle(new Vehicle("A102", 40, bus1));
    sim.addVehicle(new Vehicle("A201", 45, bus2));
    sim.addVehicle(new Vehicle("A202", 45, bus2));
    sim.addVehicle(new Vehicle("A301", 35, bus3));
    sim.addVehicle(new Vehicle("A401", 55, bus4));

    sim.addVehicle(new Vehicle("T101", 80, tram1));
    sim.addVehicle(new Vehicle("T102", 80, tram1));
    sim.addVehicle(new Vehicle("T201", 70, tram2));
    sim.addVehicle(new Vehicle("T301", 90, tram3));

    // Запуск интерактивной симуляции на 24 часа (1440 минут)
    sim.runInteractive(24);

    delete busNetwork;
    delete tramNetwork;
    delete cityMap;

    return 0;
}