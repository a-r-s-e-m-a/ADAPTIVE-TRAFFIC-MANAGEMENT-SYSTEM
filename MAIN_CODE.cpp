d#include <iostream>
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

void db_connect()
{
    if(sqlite3_open("traffic.db", &database) == SQLITE_OK)
        cout << "Database Connected Successfully.\n";
    else
        cout << "Database Connection Failed.\n";
}

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

int green_time(int vehicles)
{
    if(vehicles > 40) return 60;
    if(vehicles > 20) return 45;
    return 30;
}
void update_wait_times()
{
    for(int i = 0; i < 4; i++)
    {
        if(vehicle_counts[i] == 0)
            wait_times[i] = 0;
    }
}

void update_lane(int lane)
{
    vehicle_counts[lane] -= 20;

    if(vehicle_counts[lane] < 0)
        vehicle_counts[lane] = 0;

    wait_times[lane] = 0;

    sync_struct();
    sync_arrays();
}

void increase_wait()
{
    for(int i = 0; i < 4; i++)
    {
        if(vehicle_counts[i] > 0)
        {
            if(wait_times[i] == 0)
                wait_times[i] = 1;
            else
                wait_times[i]++;
        }
    }
}

void reset_lane_wait(int lane)
{
    wait_times[lane] = 0;
}

void reduce_vehicles(int lane)
{
    vehicle_counts[lane] -= 20;

    if(vehicle_counts[lane] < 0)
        vehicle_counts[lane] = 0;
}

bool lane_empty(int lane)
{
    return vehicle_counts[lane] == 0;
}

int total_vehicles()
{
    int total = 0;

    for(int i = 0; i < 4; i++)
        total += vehicle_counts[i];

    return total;
}

