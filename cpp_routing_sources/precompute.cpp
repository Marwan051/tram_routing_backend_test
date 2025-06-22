#include <fstream>
#include <iostream>
#include "json.hpp"
#include "helper.h"
using json = nlohmann::json;

void precomputeAllRoutes(const string& outputFilename = "all_routes.json") {

    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    // Read stop_times.csv
    string stopTimesContent = readFileContent("gtfs/stop_times.txt");
    if (stopTimesContent.empty()) {
        cerr << "Failed to read stop_times.csv. Using empty data." << endl;
    }
    map<string, int> linePrices = {
        {"B1", 5}, {"B2", 5}, {"Y1", 10}, {"Y2", 10}
    };

    // Create a set of all unique station IDs
    set<long long> allStations;
    for (const auto& trip : trips) {
        for (auto stationId : trip) {
            allStations.insert(stationId);
        }
    }

    // Initialize the route finder
    TramRouteFinder finder(trips, tripNames, linePrices, stationNames, stopTimesContent);

    json allRoutes;

    // Process each pair of stations
    for (auto srcIt = allStations.begin(); srcIt != allStations.end(); ++srcIt) {
        long long srcId = *srcIt;
        json srcRoutes;
        
        for (auto destIt = allStations.begin(); destIt != allStations.end(); ++destIt) {
            long long destId = *destIt;
            
            // Skip if source and destination are the same
            if (srcId == destId) continue;
            
            // Find the route
            RouteResult result = finder.findRoute(srcId, destId);
            
            // Convert to JSON and store
            string routeJson = finder.resultToJson(result);
            srcRoutes[to_string(destId)] = json::parse(routeJson);
        }
        
        // Add all routes from this source station to the main object
        allRoutes[to_string(srcId)] = srcRoutes;
    }

    // Write the complete JSON to file
    ofstream outFile(outputFilename);
    if (outFile.is_open()) {
        outFile << allRoutes.dump(2);
        outFile.close();
        cout << "Successfully precomputed all routes to " << outputFilename << endl;
    } else {
        cerr << "Failed to open output file: " << outputFilename << endl;
    }
}

int main() {
    precomputeAllRoutes();
    return 0;
}