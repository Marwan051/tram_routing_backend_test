#include <bits/stdc++.h>
#include "helper.h"
using namespace std;
int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <startId> <targetId> <type: realtime|precomputed>\n";
        return 1;
    }

    long long startId, targetId;
    try {
        startId  = stoll(argv[1]);
        targetId = stoll(argv[2]);
    } catch (const exception& e) {
        cerr << "Invalid ID(s): " << e.what() << "\n";
        return 1;
    }

    string mode = argv[3];
    // load stop_times for realtime only
    string stopTimesContent;
    if (mode == "realtime") {
        stopTimesContent = readFileContent("gtfs/stop_times.txt");
        if (stopTimesContent.empty()) {
            cerr << "Failed to read stop_times.txt. Using empty data.\n";
        }
    }

    map<string,int> linePrices = {{"B1",5},{"B2",5},{"Y1",10},{"Y2",10}};
    TramRouteFinder finder(trips, tripNames, linePrices, stationNames, stopTimesContent);

    if (mode == "realtime") {
        auto result = finder.findRoute(startId, targetId);
        if (!result.found) {
            cout << "{\"found\":false,\"error\":\"" << result.error << "\"}\n";
            return 1;
        }
        cout << finder.resultToJson(result) << "\n";
    }
    else if (mode == "precomputed") {
        // all_routes.json must be keyed by ID strings
        cout << finder.findPrecomputedRoute(startId, targetId, "all_routes.json") << "\n";
    }
    else {
        cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}