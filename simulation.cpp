#include "seed.cpp"
#include "peer.cpp"

using namespace std;

void startSeedNode(string ip, int port) {
    cout << "Starting SeedNode on " << ip << ":" << port << '\n';
    SeedNode seed(ip, port);
    seed.run();
}

void startPeerNode(int port) {
    cout << "[DEBUG] Creating PeerNode on port " << port << '\n';
    PeerNode peer(port);
    peer.run();
}

int main() {
    vector<thread> nodes;
    string seedIp = "172.31.69.51";
    ofstream logFile("peer_network.txt", ios::app);

    vector<int> seedPorts = {10000, 10001, 10002};
    for (int port : seedPorts) {
        string portMsgTemp = "[DEBUG] Launching SeedNode thread for port: ";
        logFile << portMsgTemp << port << "\n";
        cout << portMsgTemp << port << '\n';
        nodes.emplace_back(startSeedNode, seedIp, port);
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    vector<int> peerPorts = {6000, 6001, 6002};
    for (int port : peerPorts) {
        cout << "[DEBUG] Launching PeerNode thread for port: " << port << endl;
        nodes.emplace_back(startPeerNode, port);
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    this_thread::sleep_for(chrono::seconds(5));
    cout << "[DEBUG] Attempting to send message from PeerNode 6000" << endl;

    PeerNode sender(6000);
    sender.connectToPeer("172.31.74.146", 6000);

    for (auto& node : nodes) {
        cout << "[DEBUG] Joining thread" << endl;
        node.join();
    }

    return 0;
}