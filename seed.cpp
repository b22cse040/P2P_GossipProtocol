#include <bits/stdc++.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
using namespace std;

class SeedNode {
    private:
        string seed_ip;
        int port;
        vector<string> peers;
        mutex peers_mutex;
        ofstream log_file;

        void log_message(string& message){
            lock_guard<mutex> lock(peers_mutex);
            log_file << message << '\n';
            cout << message << '\n';
        }

        void manage_connection(int client_socket, string client_ip){
            char buffer[1024] = {0};
            while(1){
                int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if(bytes_received <= 0) break;

                buffer[bytes_received] = '\0'; // Ending the message on \0
                string data(buffer);

                lock_guard<mutex> lock(peers_mutex);
                if(!data.rfind("Dead Node", 0)) log_message(data);

                else{
                    size_t pos = data.find(":");
                    if(pos != string::npos){ // If ":" is found, because string::npos can be seen as the end of string
                        string new_peer = client_ip + ":" + data.substr(pos + 1);
                        peers.push_back(new_peer);
                        new_peer = "Connected to " + new_peer;
                        log_message(new_peer);

                        string peer_list;
                        for(const auto& peer : peers) peer_list += peer + ",";
                        send(client_socket, peer_list.c_str(), peer_list.length(), 0);
                    }
                }
            }

            close(client_socket); 
        }

    public:
        SeedNode(string ip, int port) : seed_ip(move(ip)), port(port), log_file("seed_log.txt", ios::app){};

        void start(){
            int server_socket = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM suggests a TCP Network
            if(server_socket < 0){
                cerr << "Socket creation failed!" << endl;
                return;
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, seed_ip.c_str(), &server_addr.sin_addr);

            if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
                cerr << "Binding Failed" << '\n';
                return;
            }

            if(listen(server_socket, 5) == -1){
                cerr << "Listening Failed!" << '\n';
                return;
            }

            cout << "Seed Node listening.." << "\n";
            while(1){
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
                if(client_socket == -1){
                    cerr << "Failed to accept connection!" << '\n';
                    continue;
                }

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET6_ADDRSTRLEN);
                thread(&SeedNode::manage_connection, this, client_socket, string(client_ip)).detach();
            }

            close(server_socket);
        }
};