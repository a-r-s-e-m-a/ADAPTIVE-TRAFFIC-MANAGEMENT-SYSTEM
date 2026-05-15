// ================= ADAPTIVE TRAFFIC MANAGEMENT SYSTEM =================
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include "sqlite3.h"

using namespace std;

sqlite3* database;

struct Lane
{
    string name;
    int vehicles;
    int wait_time;
};

Lane lanes[4];

string lane_names[4] = {"North", "South", "East", "West"};

int vehicle_counts[4] = {0};
int wait_times[4] = {0};

int history[4][24] = {0};
int history_index = 0;

bool emergency = false;
int emergency_lane = -1;

int cycle_number = 1;

           // STRUCT SYNC
void sync_struct()
{
    for(int i = 0; i < 4; i++)
    {
        lanes[i].name = lane_names[i];
        lanes[i].vehicles = vehicle_counts[i];
        lanes[i].wait_time = wait_times[i];
    }
}

void sync_arrays()
{
    for(int i = 0; i < 4; i++)
    {
        vehicle_counts[i] = lanes[i].vehicles;
        wait_times[i] = lanes[i].wait_time;
    }
}

    //  DATABASE CONNECT
void db_connect()
{
    if(sqlite3_open("traffic.db", &database) == SQLITE_OK)
        cout << "Database Connected Successfully.\n";
    else
        cout << "Database Connection Failed.\n";
}

   //DATABASE TABLE CREATION
void db_create()
{
    string sql =
    "CREATE TABLE IF NOT EXISTS traffic_log ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "lane TEXT,"
    "vehicles INTEGER,"
    "wait_time INTEGER,"
    "green_time INTEGER,"
    "event TEXT,"
    "cycle INTEGER);";

    sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr);

    string sql2 =
    "CREATE TABLE IF NOT EXISTS current_state ("
    "lane TEXT,"
    "vehicles INTEGER,"
    "wait_time INTEGER);";

    sqlite3_exec(database, sql2.c_str(), nullptr, nullptr, nullptr);
}

        // SAVE STATE
void save_current_state()
{
    sqlite3_exec(database, "DELETE FROM current_state;", nullptr, nullptr, nullptr);

    for(int i = 0; i < 4; i++)
    {
        string sql =
        "INSERT INTO current_state VALUES('"
        + lane_names[i] + "',"
        + to_string(vehicle_counts[i]) + ","
        + to_string(wait_times[i]) + ");";

        sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr);
    }
}

    // LOAD STATE
int load_index = 0;

int load_callback(void*, int, char** data, char**)
{
    if(data[0] && data[1] && data[2])
    {
        vehicle_counts[load_index] = stoi(data[1]);
        wait_times[load_index] = stoi(data[2]);
    }
    load_index++;
    return 0;
}

void load_current_state()
{
    load_index = 0;

    sqlite3_exec(database,
                 "SELECT * FROM current_state;",
                 load_callback,
                 nullptr,
                 nullptr);
}

    // DATABASE INSERT
void db_insert(string lane, int vehicles, int wait, int green, string event)
{
    string sql =
    "INSERT INTO traffic_log VALUES(NULL,'"
    + lane + "',"
    + to_string(vehicles) + ","
    + to_string(wait) + ","
    + to_string(green) + ",'"
    + event + "',"
    + to_string(cycle_number) + ");";

    sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr);
}

    //  FILE LOG
void file_log_cycle(int lane)
{
    ofstream file("cycle_log.txt", ios::app);

    file << "Cycle: " << cycle_number << "\n";
    file << "Lane: " << lane_names[lane] << "\n";
    file << "Vehicles: " << vehicle_counts[lane] << "\n";
    file << "Wait: " << wait_times[lane] << "\n";
    file << "Emergency: " << (emergency ? "YES" : "NO") << "\n";
    file << "----------------------\n";

    file.close();
}

    // INPUT TRAFFIC (UPDATED AS REQUESTED)
void input_traffic()
{
    cout << "\n--- VEHICLE INPUT ---\n";

    for(int i = 0; i < 4; i++)
    {
        cout << "Enter vehicles at " << lane_names[i] << ": ";
        cin >> vehicle_counts[i];

        history[i][history_index % 24] = vehicle_counts[i];
    }

    history_index++;

    sync_struct();
}

    // EMERGENCY CHECK
void emergency_check()
{
    emergency = false;

    for(int i = 0; i < 4; i++)
    {
        if(vehicle_counts[i] >= 40)
        {
            emergency = true;
            emergency_lane = i;
            return;
        }
    }
}

    // DECISION
int decide_lane()
{
    if(emergency)
        return emergency_lane;

    int best = 0;

    for(int i = 1; i < 4; i++)
    {
        if(vehicle_counts[i] > vehicle_counts[best])
            best = i;
    }

    return best;
}

    // GREEN TIME
int green_time(int vehicles)
{
    if(vehicles > 40) return 60;
    if(vehicles > 20) return 45;
    return 30;
}

    // WAIT UPDATE
void update_wait_times()
{
    for(int i = 0; i < 4; i++)
    {
        if(vehicle_counts[i] == 0)
            wait_times[i] = 0;   // fully clear empty lanes
    }

}

    // UPDATE LANE
void update_lane(int lane)
{
    vehicle_counts[lane] -= 20;

    if(vehicle_counts[lane] < 0)
        vehicle_counts[lane] = 0;

    wait_times[lane] = 0;

    sync_struct();
    sync_arrays();
}

    // DASHBOARD
void show_status()
{
    cout << "\n--- DASHBOARD ---\n";

    for(int i = 0; i < 4; i++)
    {
        cout << lane_names[i]
             << " | Vehicles: " << vehicle_counts[i]
             << " | Wait: " << wait_times[i] << "\n";
    }
}

    //  DECISION BREAKDOWN
void show_decision(int lane)
{
    cout << "\n--- DECISION BREAKDOWN ---\n";

    for(int i = 0; i < 4; i++)
    {
        cout << lane_names[i]
             << " | Vehicles: " << vehicle_counts[i]
             << " | Wait: " << wait_times[i];

        if(i == lane)
            cout << " <-- SELECTED";

        cout << "\n";
    }

    cout << "\nSELECTED: " << lane_names[lane] << "\n";
}

    // PERFORMANCE
int performance_score()
{
    int total = 0;

    for(int i = 0; i < 4; i++)
        total += vehicle_counts[i];

    int score = 100 - (total / 10);

    if(score < 0) score = 0;

    return score;
}

    // CALLBACK
int callback(void*, int col, char** data, char** colName)
{
    for(int i = 0; i < col; i++)
    {
        cout << colName[i] << ": "
             << (data[i] ? data[i] : "0")
             << " | ";
    }
    cout << endl;
    return 0;
}

    //  ANALYTICS
void analytics_menu()
{
    sync_struct();

    cout << "\n====================================\n";
    cout << "       ANALYTICS DASHBOARD          \n";
    cout << "====================================\n";

    //FULL TRAFFIC LOGS
    cout << "\n--- FULL TRAFFIC LOGS ---\n";
    cout << "ID | LANE  | VEHICLES | WAIT(s) | GREEN(s) | EVENT     | CYCLE\n";
    cout << "----------------------------------------------------------------\n";
    sqlite3_exec(database,
    "SELECT * FROM traffic_log;",
    [](void*, int, char** d, char**) -> int
    {
        cout << d[0] << "  | "
             << d[1] << "  | "
             << d[2] << "        | "
             << d[3] << "       | "
             << d[4] << "       | "
             << d[5] << "   | "
             << d[6] << "\n";
        return 0;
    },
    nullptr, nullptr);

    // AVERAGE VEHICLES PER LANE
    cout << "\n--- AVERAGE VEHICLES PER LANE ---\n";
    cout << "LANE  | AVG VEHICLES\n";
    cout << "---------------------\n";
    sqlite3_exec(database,
    "SELECT lane, ROUND(AVG(vehicles),1) FROM traffic_log GROUP BY lane ORDER BY lane;",
    [](void*, int, char** d, char**) -> int
    {
        cout << d[0] << " | " << d[1] << "\n";
        return 0;
    },
    nullptr, nullptr);

    //BUSIEST LANE RANKED
    cout << "\n--- BUSIEST LANE (RANKED) ---\n";
    cout << "LANE  | MAX VEHICLES\n";
    cout << "---------------------\n";
    sqlite3_exec(database,
    "SELECT lane, MAX(vehicles) FROM traffic_log GROUP BY lane ORDER BY MAX(vehicles) DESC;",
    [](void*, int, char** d, char**) -> int
    {
        cout << d[0] << " | " << d[1] << "\n";
        return 0;
    },
    nullptr, nullptr);

    //EMERGENCY COUNT
    cout << "\n--- EMERGENCY EVENTS ---\n";
    cout << "TOTAL: ";
    sqlite3_exec(database,
    "SELECT COUNT(*) FROM traffic_log WHERE event='EMERGENCY';",
    [](void*, int, char** d, char**) -> int
    {
        cout << d[0] << "\n";
        return 0;
    },
    nullptr, nullptr);

}

    // RUN CYCLE
void run_cycle()
{
    cout << "\n====================\n";
    cout << "CYCLE " << cycle_number << "\n";
    cout << "====================\n";

    update_wait_times();
    emergency_check();

    int lane = decide_lane();

    if(lane == -1)
    {
        cout << "\nNO VEHICLES\n";
        return;
    }

    int time = green_time(vehicle_counts[lane]);

    cout << "\nGREEN LIGHT: " << lane_names[lane] << "\n";
    cout << "Time: " << time << " sec\n";

    db_insert(lane_names[lane],
              vehicle_counts[lane],
              wait_times[lane],
              time,
              emergency ? "EMERGENCY" : "NORMAL");

    file_log_cycle(lane);

    show_decision(lane);

    for(int i = 0; i < 4; i++)
    {
        if(i != lane && vehicle_counts[i] > 0)
        {
            if(wait_times[i] == 0)
                wait_times[i] = 1;
            else
                wait_times[i]++;
        }
    }

    wait_times[lane] = 0;

    // VEHICLE REDUCTION
    vehicle_counts[lane] -= 20;
    if(vehicle_counts[lane] < 0)
        vehicle_counts[lane] = 0;

    // SYNC
    sync_struct();
    sync_arrays();

    show_status();

    cout << "\nScore: " << performance_score() << "/100\n";

    save_current_state();

    cycle_number++;
}


    //  AUTO MODE
bool all_clear()
{
    for(int i = 0; i < 4; i++)
        if(vehicle_counts[i] > 0)
            return false;

    return true;
}

void auto_mode()
{
    input_traffic();

    save_current_state();

    char choice;

    while(true)
    {
        for(int i = 0; i < 5; i++)
        {
            load_current_state();

            if(all_clear())
            {
                cout << "\nALL LANES CLEARED - SYSTEM STOPPED\n";
                return;
            }

            run_cycle();

            save_current_state();

            if(all_clear())
            {
                cout << "\nALL LANES CLEARED - SYSTEM STOPPED\n";
                return;
            }
        }

        cout << "\n 5 CYCLES COMPLETED\n";

        cout << "Continue another 5 cycles? (y/n): ";
        cin >> choice;

        if(choice != 'y')
        {
            cout << "\nAUTO MODE STOPPED BY USER\n";
            return;
        }
    }
}


    // MANUAL MODE
void manual_mode()
{
    char c;

    do
    {
        input_traffic();
        run_cycle();

        cout << "\nContinue? (y/n): ";
        cin >> c;

    } while(c == 'y');
}

    //  MAIN
int main()
{
    db_connect();
    db_create();

    int choice;
    cout << "\n====================================\n";
    cout << " ADAPTIVE TRAFFIC MANAGEMENT SYSTEM\n";
    cout << "====================================\n";

    while(true)
    {
        cout << "\n1. AUTO MODE\n";
        cout << "   -> Runs automatically\n";
        cout << "   -> Stops every 5 cycles for confirmation\n";

        cout << "\n2. MANUAL MODE\n";
        cout << "   -> User enters traffic every cycle\n";

        cout << "\n3. ANALYTICS DASHBOARD\n";
        cout << "   -> Shows database reports\n";

        cout << "\n4. EXIT\n";


        cout << "\n====Choice: ";
        cin >> choice;

        if(choice == 1)
            auto_mode();
        else if(choice == 2)
            manual_mode();
        else if(choice == 3)
            analytics_menu();
        else
            break;
    }

    sqlite3_close(database);

    cout << "\nSystem Shutdown Complete\n";
    return 0;
}
