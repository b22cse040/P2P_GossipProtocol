#include <bits/stdc++.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>

using namespace std;

class SeedNode {
private:
    string seed_ip;
    int port;
    vector<string> peers;
    mutex peers_mutex;
    ofstream logFile;

    void log_message(const string& message) {
        cout<<message<<endl;
        lock_guard<mutex> lock(peers_mutex);
        if (!logFile.is_open()) return;
        logFile << message << '\n';
        cout << message << '\n';
    }

    void remove_dead_peer(const string& dead_peer) {
        lock_guard<mutex> lock(peers_mutex);
        peers.erase(remove(peers.begin(), peers.end(), dead_peer), peers.end());
        log_message("[INFO] Removed Dead Peer: " + dead_peer);
    }

    vector<string> get_power_law_peers() {
      
        vector<string> subset;
        if (peers.empty()) return subset;
        
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(1, max(1, (int)peers.size()));  

        int count = max(1, dist(gen) / 2);
        shuffle(peers.begin(), peers.end(), gen);

        for (int i = 0; i < count; ++i) {
            subset.push_back(peers[i]);
        }
        return subset;
     
    }

    void manage_connection(int client_socket, string client_ip) {
        char buffer[1024] = {0};
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            close(client_socket);
            return;
        }

        buffer[bytes_received] = '\0';
        string data(buffer);

      
        if (data.rfind("Dead Node", 0) == 0) {
            log_message("[ALERT] " + data);
            remove_dead_peer(data.substr(10));
        } else if (data.rfind("Register:", 0) == 0) {

            cout<<"Registering"<<endl;
            string peer_port = data.substr(9);
            string new_peer = client_ip + ":" + peer_port;
            peers.push_back(new_peer);
            cout<<"Peer List"<<peers.size()<<endl;
          
            string peer_list;
            vector<string> selected_peers = get_power_law_peers();
            cout<<"List Size : "<<selected_peers.size()<<endl;
            for (const auto& peer : selected_peers) {
                peer_list += peer + ",";
            }
            if (!peer_list.empty()) peer_list.pop_back(); 
            
            send(client_socket, peer_list.c_str(), peer_list.length(), 0);
            log_message("[INFO] Sent peer list to " + new_peer + ": " + peer_list);
        }
        close(client_socket);
    }

public:
    SeedNode(string ip, int p) : seed_ip(move(ip)), port(p) {
      
    }

    void run() {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "Socket creation failed!" << endl;
            return;
        }

        // ✅ Enable SO_REUSEADDR to prevent binding errors
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, seed_ip.c_str(), &server_addr.sin_addr);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            cerr << "[ERROR] Binding Failed on port " << port << '\n';
            close(server_socket);
            return;
        }

        if (listen(server_socket, 5) == -1) {
            cerr << "[ERROR] Listening Failed on port " << port << '\n';
            close(server_socket);
            return;
        }

        cout << "[INFO] Seed Node listening on " << seed_ip << ":" << port << "\n";
        while (true) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket == -1) continue;

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            thread(&SeedNode::manage_connection, this, client_socket, string(client_ip)).detach();
        }
        close(server_socket);
    }
};
