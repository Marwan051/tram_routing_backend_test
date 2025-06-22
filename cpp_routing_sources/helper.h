#include <bits/stdc++.h>
#include "json.hpp" 
using json = nlohmann::json;
using namespace std;

constexpr int INF = 1e9;

struct Edge
{
    int to;
    int travelTime;
    Edge(int t, int time) : to(t), travelTime(time) {}
};

struct RouteStep
{
    string action;
    long long stationId;
    string stationName;
    string line;
    int cost;
};

struct DirectRoute
{
    string line;
    int cost;
    int totalTime;
    vector<RouteStep> shortSteps;
    vector<RouteStep> fullSteps;
};

struct OptimalRoute
{
    int totalCost;
    int totalTime;
    vector<RouteStep> shortSteps;
    vector<RouteStep> fullSteps;
};

struct RouteResult
{
    bool found;
    OptimalRoute optimalRoute;
    vector<DirectRoute> directRoutes;
    string error;
};

struct StopTime
{
    string tripId;
    int arrivalTime;
    int departureTime;
    long long stopId;
    int stopSequence;
};

class TramRouteFinder
{
private:
    vector<vector<long long>> trips;
    vector<string> tripNames;
    map<string, int> linePrices;
    map<long long, string> stationNames;
    map<long long, int> stopToIdx;
    map<int, long long> idxToStopId;
    vector<vector<Edge>> adjacencyList;
    map<int, vector<string>> stationLines;
    map<string, vector<StopTime>> tripStopTimes;
    map<pair<long long, long long>, int> stopPairTimes;

    int parseTime(const string &timeStr)
    {
        int hours = stoi(timeStr.substr(0, 2));
        int minutes = stoi(timeStr.substr(3, 2));
        return hours * 60 + minutes;
    }

    void loadStopTimes(const string &stopTimesContent)
    {
        istringstream ss(stopTimesContent);
        string line;
        getline(ss, line); // Skip header

        while (getline(ss, line))
        {
            istringstream lineStream(line);
            string field;
            vector<string> fields;

            while (getline(lineStream, field, ','))
            {

                fields.push_back(field);
            }

            if (fields.size() >= 4)
            {
                StopTime stopTime;
                stopTime.tripId = fields[0];
                stopTime.arrivalTime = parseTime(fields[1]);
                stopTime.departureTime = parseTime(fields[2]);
                stopTime.stopId = stoll(fields[3]);

                stopTime.stopSequence = stoi(fields[4]);

                tripStopTimes[stopTime.tripId].push_back(stopTime);
            }
        }

        for (auto &pair : tripStopTimes)
        {
            sort(pair.second.begin(), pair.second.end(),
                 [](const StopTime &a, const StopTime &b)
                 {
                     return a.stopSequence < b.stopSequence;
                 });
        }

        for (const auto &pair : tripStopTimes)
        {
            const vector<StopTime> &stops = pair.second;
            for (int i = 0; i < stops.size() - 1; i++)
            {
                long long fromStop = stops[i].stopId;
                long long toStop = stops[i + 1].stopId;
                int travelTime = stops[i + 1].arrivalTime - stops[i].departureTime;

                auto key = make_pair(fromStop, toStop);
                if (stopPairTimes.find(key) == stopPairTimes.end() ||
                    stopPairTimes[key] > travelTime)
                {
                    stopPairTimes[key] = travelTime;
                }

                auto reverseKey = make_pair(toStop, fromStop);
                if (stopPairTimes.find(reverseKey) == stopPairTimes.end() ||
                    stopPairTimes[reverseKey] > travelTime)
                {
                    stopPairTimes[reverseKey] = travelTime;
                }
            }
        }
    }

    void initializeData()
    {
        set<long long> allStations;
        for (const auto &trip : trips)
        {
            for (auto stationId : trip)
            {
                allStations.insert(stationId);
            }
        }

        int idx = 0;
        for (auto stationId : allStations)
        {
            stopToIdx[stationId] = idx;
            idxToStopId[idx] = stationId;
            idx++;
        }

        int n = allStations.size();
        adjacencyList.resize(n);

        for (int tripIdx = 0; tripIdx < trips.size(); tripIdx++)
        {
            const auto &trip = trips[tripIdx];
            const string &line = tripNames[tripIdx];

            for (int i = 0; i < trip.size(); i++)
            {
                long long stationId = trip[i];
                int u = stopToIdx[stationId];
                stationLines[u].push_back(line);

                if (i + 1 < trip.size())
                {
                    long long nextStationId = trip[i + 1];
                    int v = stopToIdx[nextStationId];

                    int travelTime = 1;
                    auto key = make_pair(stationId, nextStationId);
                    if (stopPairTimes.find(key) != stopPairTimes.end())
                    {
                        travelTime = stopPairTimes[key];
                    }

                    adjacencyList[u].push_back({v, travelTime});
                    adjacencyList[v].push_back({u, travelTime});
                }
            }
        }
    }
    private:
    vector<RouteStep> generateShortSteps(long long srcId, long long destId, 
                                       const vector<pair<long long, string>>& path) {
        vector<RouteStep> steps;
        if (path.empty()) return steps;

        // Board at first station
        steps.push_back({
            "board", path[0].first, stationNames[path[0].first], path[0].second, linePrices[path[0].second]
        });

        string currentLine = path[0].second;
        
        for (size_t i = 1; i < path.size(); i++) {
            auto [stationId, line] = path[i];
            
            // Handle line changes
            if (line != currentLine) {
                steps.push_back({
                    "transfer", stationId, stationNames[stationId], currentLine + " -> " + line, 0
                });
                steps.push_back({
                    "board", stationId, stationNames[stationId], line, linePrices[line]
                });
                currentLine = line;
            }
            
            // Last station is always arrive
            if (i == path.size() - 1) {
                steps.push_back({
                    "arrive", stationId, stationNames[stationId], currentLine, 0
                });
            }
        }

        return steps;
    }

    vector<RouteStep> generateFullSteps(long long srcId, long long destId,
                                      const vector<pair<long long, string>>& path) {
        vector<RouteStep> steps;
        if (path.empty()) return steps;

        // Board at first station
        steps.push_back({
            "board", path[0].first, stationNames[path[0].first], path[0].second, linePrices[path[0].second]
        });

        string currentLine = path[0].second;
        
        for (size_t i = 1; i < path.size(); i++) {
            auto [stationId, line] = path[i];
            
            // Handle line changes
            if (line != currentLine) {
                steps.push_back({
                    "transfer", stationId, stationNames[stationId], currentLine + " -> " + line, 0
                });
                steps.push_back({
                    "board", stationId, stationNames[stationId], line, linePrices[line]
                });
                currentLine = line;
            }
            // Intermediate stations (not last)
            else if (i < path.size() - 1) {
                steps.push_back({
                    "pass", stationId, stationNames[stationId], currentLine, 0
                });
            }
            
            // Last station is always arrive
            if (i == path.size() - 1) {
                steps.push_back({
                    "arrive", stationId, stationNames[stationId], currentLine, 0
                });
            }
        }

        return steps;
    }
    vector<DirectRoute> findDirectRoutes(long long srcId, long long destId) {
        vector<DirectRoute> directRoutes;

        for (int i = 0; i < trips.size(); i++) {
            const string &line = tripNames[i];
            const auto &trip = trips[i];

            auto srcIt = find(trip.begin(), trip.end(), srcId);
            auto destIt = find(trip.begin(), trip.end(), destId);

            if (srcIt != trip.end() && destIt != trip.end()) {
                int cost = linePrices[line];
                int totalTime = 0;
                int srcPos = srcIt - trip.begin();
                int destPos = destIt - trip.begin();

                // Calculate total time
                if (srcPos < destPos) {
                    for (int j = srcPos; j < destPos; j++) {
                        auto key = make_pair(trip[j], trip[j + 1]);
                        totalTime += (stopPairTimes.find(key) != stopPairTimes.end()) ? stopPairTimes[key] : 5;
                    }
                } else {
                    for (int j = srcPos; j > destPos; j--) {
                        auto key = make_pair(trip[j], trip[j - 1]);
                        totalTime += (stopPairTimes.find(key) != stopPairTimes.end()) ? stopPairTimes[key] : 5;
                    }
                }

                // Prepare path for unified step generation
                vector<pair<long long, string>> path;
                int step = (srcPos < destPos) ? 1 : -1;
                for (int j = srcPos; j != destPos + step; j += step) {
                    path.emplace_back(trip[j], line);
                }

                DirectRoute route;
                route.line = line;
                route.cost = cost;
                route.totalTime = totalTime;
                route.shortSteps = generateShortSteps(srcId, destId, path);
                route.fullSteps = generateFullSteps(srcId, destId, path);
                directRoutes.push_back(route);
            }
        }

        return directRoutes;
    }
OptimalRoute dijkstraOptimal(long long srcId, long long destId) {
    int src = stopToIdx[srcId];
    int dest = stopToIdx[destId];
    int n = adjacencyList.size();

    map<pair<int, string>, tuple<int, int, int, string>> state;
    priority_queue<tuple<int, int, int, string>, vector<tuple<int, int, int, string>>, greater<>> pq;

    // Initialize with all possible lines at source station
    for (const string &line : stationLines.at(src)) {
        int fare = linePrices[line];
        state[{src, line}] = {fare, 0, -1, ""};
        pq.emplace(fare, 0, src, line);
    }

    // Dijkstra's algorithm
    while (!pq.empty()) {
        auto [cost, time, u, currentLine] = pq.top();
        pq.pop();

        // Skip if we already found a better way
        pair<int, int> currentState = {get<0>(state[{u, currentLine}]), get<1>(state[{u, currentLine}])};
        if (state.count({u, currentLine}) && currentState < make_pair(cost, time)) {
            continue;
        }

        // Explore neighbors
        for (const Edge &edge : adjacencyList[u]) {
            int v = edge.to;
            int newTime = time + edge.travelTime;

            // Case 1: Continue on same line
            if (find(stationLines.at(v).begin(), stationLines.at(v).end(), currentLine) != stationLines.at(v).end()) {
                int newCost = cost;
                pair<int, string> newState = {v, currentLine};

                if (!state.count(newState) ||
                    make_pair(newCost, newTime) < make_pair(get<0>(state[newState]), get<1>(state[newState]))) {
                    state[newState] = {newCost, newTime, u, currentLine};
                    pq.emplace(newCost, newTime, v, currentLine);
                }
            }

            // Case 2: Transfer to another line
            for (const string &newLine : stationLines.at(v)) {
                if (newLine != currentLine) {
                    int transferFare = linePrices[newLine];
                    int newCost = cost + transferFare;
                    pair<int, string> newState = {v, newLine};

                    if (!state.count(newState) ||
                        make_pair(newCost, newTime) < make_pair(get<0>(state[newState]), get<1>(state[newState]))) {
                        state[newState] = {newCost, newTime, u, currentLine};
                        pq.emplace(newCost, newTime, v, newLine);
                    }
                }
            }
        }
    }

    // Find best path to destination
    int bestCost = INF, bestTime = INF;
    string bestLine;
    for (const string &line : stationLines.at(dest)) {
        if (state.count({dest, line})) {
            auto [c, t, prevStation, prevLine] = state[{dest, line}];
            if (make_pair(c, t) < make_pair(bestCost, bestTime)) {
                bestCost = c;
                bestTime = t;
                bestLine = line;
            }
        }
    }

    if (bestCost == INF) return {INF, INF, {}, {}};

    // Reconstruct path
    vector<pair<long long, string>> path;
    int currentStation = dest;
    string currentPathLine = bestLine;

    while (currentStation != -1) {
        auto [cost, time, prevStation, prevLine] = state[{currentStation, currentPathLine}];
        path.emplace_back(idxToStopId[currentStation], currentPathLine);
        currentStation = prevStation;
        currentPathLine = prevLine;
    }

    reverse(path.begin(), path.end());

    // Generate steps
    OptimalRoute result;
    result.totalCost = bestCost;
    result.totalTime = bestTime;
    result.shortSteps = generateShortSteps(srcId, destId, path);
    result.fullSteps = generateFullSteps(srcId, destId, path);

    return result;
}

public:
    TramRouteFinder(const vector<vector<long long>> &tripData,
                    const vector<string> &tripNameData,
                    const map<string, int> &linePriceData,
                    const map<long long, string> &stationNameData,
                    const string &stopTimesContent = "")
    {
        trips = tripData;
        tripNames = tripNameData;
        linePrices = linePriceData;
        stationNames = stationNameData;

        if (!stopTimesContent.empty()) loadStopTimes(stopTimesContent);

        initializeData();
    }

    RouteResult findRoute(long long startStationId, long long targetStationId)
    {
        RouteResult result;
        result.found = false;

        if (stopToIdx.find(startStationId) == stopToIdx.end()) {result.error = "Start station ID " + to_string(startStationId) + " not found"; return result;}

        if (stopToIdx.find(targetStationId) == stopToIdx.end()){result.error = "Target station ID " + to_string(targetStationId) + " not found"; return result;}

        if (startStationId == targetStationId) {result.error = "Start and target stations are the same"; return result;}

        result.directRoutes = findDirectRoutes(startStationId, targetStationId);
        result.optimalRoute = dijkstraOptimal(startStationId, targetStationId);

        if (result.optimalRoute.totalCost == INF) {result.error = "No route found between these stations"; return result;}

        result.found = true;
        return result;
    }

    string resultToJson(const RouteResult &result)
    {
        json j;
        if (!result.found) {j["found"] = false; j["error"] = result.error;}
        else
        {
            j["found"] = true;

            // optimalRoute
            json opt;
            opt["cost"] = result.optimalRoute.totalCost;
            opt["time"] = result.optimalRoute.totalTime;

            // shortSteps
            for (auto &step : result.optimalRoute.shortSteps)
            {
                opt["shortSteps"].push_back({{"action", step.action},
                                             {"stationId", step.stationId},
                                             {"stationName", step.stationName},
                                             {"line", step.line},
                                             {"cost", step.cost}});
            }

            // fullSteps
            for (auto &step : result.optimalRoute.fullSteps)
            {
                opt["fullSteps"].push_back({{"action", step.action},
                                            {"stationId", step.stationId},
                                            {"stationName", step.stationName},
                                            {"line", step.line},
                                            {"cost", step.cost}});
            }

            j["optimalRoute"] = move(opt);

            // directRoutes
            for (auto &route : result.directRoutes)
            {
                json dr;
                dr["line"] = route.line;
                dr["cost"] = route.cost;
                dr["time"] = route.totalTime;
            for (auto &step : route.shortSteps)
            {
                dr["shortSteps"].push_back({{"action", step.action},
                                             {"stationId", step.stationId},
                                             {"stationName", step.stationName},
                                             {"line", step.line},
                                             {"cost", step.cost}});
            }
            for (auto &step : route.fullSteps)
            {
                dr["fullSteps"].push_back({{"action", step.action},
                                            {"stationId", step.stationId},
                                            {"stationName", step.stationName},
                                            {"line", step.line},
                                            {"cost", step.cost}});
            }
            j["directRoutes"].push_back(move(dr));
            }
        }

        // pretty-print with 2 spaces
        return j.dump(2);
    }

    string findPrecomputedRoute(long long startStationId,
                                long long targetStationId,
                                const string &filename = "all_routes.json")
    {
        ifstream in(filename);
        if (!in.is_open()) return R"({"found":false,"error":"cannot open precomputed file"})";
        json j;
        in >> j;
        in.close();

        string s = to_string(startStationId);
        string t = to_string(targetStationId);
        if (j.contains(s) && j[s].contains(t)) return j[s][t].dump(2);
        else return R"({"found":false,"error":"no precomputed route"})";
    }
};

vector<vector<long long>> trips = {
    {316823148, 3909161113, 3880407513, 3909161121, 3909161122, 3909161123, 1886590988, 1886590968, 4792326286, 8412131579, 316824347, 5969128858, 3950015169, 316829952, 4628330183, 4748562215, 1885060212, 4002201650, 4748507684, 6095379250, 316824433, 401866306, 10970120024, 6095433434, 6095471258, 6095433435, 6095519914, 4984424561, 1263172984, 1263173062, 316824788},
    {316823148, 3909161113, 3880407513, 3909161121, 3909161122, 3909161123, 1886590988, 4002201610, 4478874013, 4478874012, 4669952291, 8412131578, 316829952, 4628330183, 4748562215, 1885060212, 4002201652, 6095358858, 316824433, 401866306, 10970120024, 6095433434, 6095471258, 6095433435, 6095519914, 4984424561, 1263172984, 1263173062, 316824788},
    {1886590988, 1886590968, 4792326286, 8412131579, 316824347, 5969128858, 3950015169, 316829952, 4628330183, 4748562215, 1885060212, 4002201650, 4748507684, 6095379250, 316824433, 401866306, 10970120024, 6095433434, 6095471258, 6095433435, 6095519914, 4984424561, 1263172984, 1263173062},
    {1886590988, 4002201610, 4478874013, 4478874012, 4669952291, 8412131578, 316829952, 4628330183, 4748562215, 1885060212, 4002201652, 6095358858, 316824433, 401866306, 10970120024, 6095433434, 6095471258, 6095433435, 6095519914, 4984424561, 1263172984, 1263173062}};
vector<string> tripNames = {"B1", "B2", "Y1", "Y2"};

map<long long, string> stationNames = {
    {316823148, "El Nasr Station"}, {316824347, "Bakus"}, {316824433, "Sporting El-Kobra Station"}, {316824788, "Ramleh Station"}, {316829952, "Bulky Station"}, {401866306, "Sporting El Soghra"}, {1263172984, "El Azarita Station"}, {1263173062, "Ibrahim Mosque Station"}, {1885060212, "Moustafa Kamel"}, {1886590968, "Gnaklis Station"}, {1886590988, "San Stefano"}, {3880407513, "Sidi Bishr"}, {3909161113, "El Suyuf Station"}, {3909161121, "El Saraya"}, {3909161122, "Luran Station"}, {3909161123, "Tharwat Station"}, {3950015169, "El Wezarah"}, {4002201610, "Qasr El-Safa"}, {4002201650, "Sidi Gaber El Sheikh"}, {4002201652, "Sidi Gaber El Mahatta"}, {4478874012, "Gleem Station"}, {4478874013, "El-Fonoon El-Gamila"}, {4628330183, "Roshdy Station"}, {4669952291, "Saba Basha Station"}, {4748507684, "Cleopatra El Kobra Station"}, {4748562215, "Mohamed Mahfouz Station"}, {4792326286, "Shots Station"}, {4984424561, "El Shahid Mustafa Zayan Station"}, {5969128858, "Fleming Station"}, {6095358858, "Cleopatra Station"}, {6095379250, "Cleopatra Alsoghra Tram Station"}, {6095433434, "Camp Chezar"}, {6095433435, "El Shatby Station"}, {6095471258, "El Gamaa"}, {6095519914, "El Shoban El Muslmen Station"}, {8412131578, "El Hadaya Station"}, {8412131579, "Safer Station"}, {10970120024, "Al Ibrahimia Station"}};

string readFileContent(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        return "";
    }

    string content, line;
    while (getline(file, line))
    {
        content += line + "\n";
    }
    file.close();
    return content;
}