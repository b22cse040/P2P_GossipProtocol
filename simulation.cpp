#include "seed.cpp"
#include "peer.cpp"

using namespace std;

void startSeedNode(string ip, int port) {
    SeedNode seed(ip, port);
    seed.run();
}

void startPeerNode(int port) {
    PeerNode peer(port);
    peer.run();
}

int main() {
    vector<thread> nodes;
    string seedIp = "172.31.74.146";
    
    vector<int> seedPorts = {10000, 10001};
    for (int port : seedPorts) {
        nodes.emplace_back(startSeedNode, seedIp, port);
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    
    vector<int> peerPorts = {6000, 6001, 6002};
    for (int port : peerPorts) {
        nodes.emplace_back(startPeerNode, port);
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    
    this_thread::sleep_for(chrono::seconds(5));
    PeerNode sender(6000);
    sender.connectToPeer("172.31.74.146:6001", "Test Message from 6000");
    
    for (auto& node : nodes) {
        node.join();
    }
    
    return 0;
}
