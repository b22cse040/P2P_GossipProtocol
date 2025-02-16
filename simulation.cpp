#include "seed.cpp"
#include "peer.cpp"
#include <fstream>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <unordered_set>
#include <random>

using namespace std;

mutex graphMutex;
mutex logMutex;

// Helper function to shuffle and pick random peers
vector<int> getRandomPeers(vector<int> peerPorts, int numConnections, int excludePort) {
    vector<int> randomPeers;
    random_device rd;
    mt19937 gen(rd());
    shuffle(peerPorts.begin(), peerPorts.end(), gen);

    for (int port : peerPorts) {
        if (port != excludePort) {
            randomPeers.push_back(port);
            if (randomPeers.size() == numConnections)
                break;
        }
    }
    return randomPeers;
}
void killRandomPeer(vector<int> peerPorts, ofstream &logFile, ofstream &graphFile) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, peerPorts.size() - 1);

    int killedPeer = peerPorts[dis(gen)];
    cout << "[ALERT] Killing Peer " << killedPeer << "!\n";
    cout << "Killing Peer" << endl;
    
    {
        lock_guard<mutex> logLock(logMutex);
        logFile << "[DEAD] Peer " << killedPeer << " has failed.\n";
        logFile.flush();  // <-- Ensure data is written immediately
    }

    {
        lock_guard<mutex> lock(graphMutex);
        graphFile << "[DEAD] Peer " << killedPeer << "\n";
        graphFile.flush();  // <-- Ensure data is written immediately
    }
}


void startSeedNode(string ip, int port, ofstream &graphFile, ofstream &logFile) {
    cout << "Starting SeedNode on " << ip << ":" << port << '\n';

    {
        lock_guard<mutex> lock(graphMutex);
        graphFile << "Seed " << port << "\n";
    }

    {
        lock_guard<mutex> lock(logMutex);
        logFile << "Seed node running on " << ip << ":" << port << "\n";
    }

    SeedNode seed(ip, port);
    seed.run();
}

void startPeerNode(int port, vector<int> allPeerPorts, ofstream &graphFile, ofstream &logFile) {
    cout << "[DEBUG] Creating PeerNode on port " << port << "\n";

    {
        lock_guard<mutex> lock(graphMutex);
        graphFile << "Peer " << port << "\n";
    }

    {
        lock_guard<mutex> lock(logMutex);
        logFile << "Peer node started on port " << port << "\n";
    }

    PeerNode peer(port);
    peer.registerWithSeeds();

    unordered_set<int> connectedPeers; // Track connections to prevent duplicates

   
    vector<int> extraPeers = getRandomPeers(allPeerPorts, 3, port);

    for (const auto &peerAddress : peer.getConnectedPeers()) {
        size_t pos = peerAddress.find(":");
        if (pos != string::npos) {
            int peerPort = stoi(peerAddress.substr(pos + 1));
            connectedPeers.insert(peerPort);
        }
    }

    for (int peerPort : extraPeers) {
        if (connectedPeers.find(peerPort) == connectedPeers.end()) {
            peer.connectToPeer("127.0.0.1", peerPort, "Additional Connection");
            connectedPeers.insert(peerPort);

            {
                lock_guard<mutex> lock(graphMutex);
                graphFile << "Connection " << port << " -> " << peerPort << "\n";
            }

            {
                lock_guard<mutex> lock(logMutex);
                logFile << "Peer " << port << " added extra connection to " << peerPort << "\n";
            }
        }
    }

   
    thread gossipThread(&PeerNode::sendGossipMessage, &peer);
    gossipThread.detach();

    peer.run(); 
}

int main() {
    vector<thread> nodes;
    string seedIp = "127.0.0.1";

    ofstream graphFile("graph.txt",ios::app);
    ofstream logFile("peer_network.txt");

    if (!graphFile || !logFile) {
        cerr << "[ERROR] Could not open graph.txt or peer_network.txt" << endl;
        return 1;
    }

    vector<int> seedPorts = {10000, 10001, 10002};
    for (int port : seedPorts) {
        nodes.emplace_back(startSeedNode, seedIp, port, ref(graphFile), ref(logFile));
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    vector<int> peerPorts = {6000, 6001, 6002, 6003, 6004, 6005};
    for (int port : peerPorts) {
        nodes.emplace_back(startPeerNode, port, peerPorts, ref(graphFile), ref(logFile));
        this_thread::sleep_for(chrono::milliseconds(500));
    }
     this_thread::sleep_for(chrono::seconds(5));
     killRandomPeer(peerPorts, logFile, graphFile);
    for (auto& node : nodes) {
        node.join();
    }

    graphFile.close();
    logFile.close();

    return 0;
}
