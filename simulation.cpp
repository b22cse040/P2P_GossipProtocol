#include "seed.cpp"
#include "peer.cpp"
#include <fstream>
#include <mutex>

using namespace std;

mutex graphMutex;  // Mutex to prevent simultaneous writes

void startSeedNode(string ip, int port, ofstream &graphFile) {
    cout << "Starting SeedNode on " << ip << ":" << port << '\n';

    // Lock graph file before writing
    {
        lock_guard<mutex> lock(graphMutex);
        graphFile << "Seed " << port << "\n";
    }

    SeedNode seed(ip, port);
    seed.run();
}

void startPeerNode(int port, ofstream &graphFile) {
    cout << "[DEBUG] Creating PeerNode on port " << port << "\n";

    // {
    //     lock_guard<mutex> lock(graphMutex);
    //     graphFile << "Peer " << port << "\n";
    // }

    PeerNode peer(port);
  
    peer.registerWithSeeds(); // Connect to seeds and get the peer list

    // 🔹 NEW: Automatically connect to all known peers
    cout<<peer.getConnectedPeers().size()<<endl;
    cout<<"line 37"<<endl;
    for (const auto &peerAddress : peer.getConnectedPeers()) {
        size_t pos = peerAddress.find(":");
        if (pos != string::npos) {
            string peerIp = peerAddress.substr(0, pos);
            int peerPort = stoi(peerAddress.substr(pos + 1));
            peer.connectToPeer(peerIp, peerPort, "Initial Connection Message");

            // Log connections
            {
                lock_guard<mutex> lock(graphMutex);
                graphFile << "Connection " << port << " -> " << peerPort << "\n";
            }
        }
    }

    // 🔹 NEW: Start Gossip Messaging in a Separate Thread
    thread gossipThread(&PeerNode::sendGossipMessage, &peer);
    gossipThread.detach(); // Keep gossip running independently

    peer.run(); // Start the peer's event loop
}

int main() {
    vector<thread> nodes;
    string seedIp = "127.0.0.1";
    ofstream logFile("peer_network.txt", ios::app);

    // Open graph.txt **once** and keep it open
    ofstream graphFile("graph.txt");
    if (!graphFile) {
        cerr << "[ERROR] Could not open graph.txt" << endl;
        return 1;
    }

    // 🔹 Step 1: Start Seed Nodes
    vector<int> seedPorts = {10000, 10001, 10002};
    for (int port : seedPorts) {
        nodes.emplace_back(startSeedNode, seedIp, port, ref(graphFile));
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    // 🔹 Step 2: Start Peer Nodes
    vector<int> peerPorts = {6000, 6001, 6002, 6003, 6004, 6005};
    for (int port : peerPorts) {
        nodes.emplace_back(startPeerNode, port, ref(graphFile));
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    for (auto& node : nodes) {
        node.join();
    }

    graphFile.close();  // Close file at the end
    return 0;
}
