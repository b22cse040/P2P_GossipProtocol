#include <bits/stdc++.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_set>
#include <queue>
using namespace std;

class PeerNode {
    private:
        mutex mtx;
        condition_variable cv;
        queue<int> jobQueue;
        set<string> seedsAddresses;
        vector<string> connectedPeers;
        vector<string> connectedSeedAddr;
        unordered_set<string> messageList;
        string myIpAddress = "172.31.74.146";
        int myPort;
        ofstream logFile;

    public:
        PeerNode(int port) : myPort(port) {
            ofstream logFile("peer_network.txt", ios::app);
            if (!logFile) {
                cerr << "[ERROR] Failed to open peer_network.txt" << endl;
                return;
            }
            logFile << "[LOG] PeerNode initialized at port " << port << endl;
            logFile.close();
        }


        void logMessage(const string& message){
            // lock_guard<mutex> lock(mtx);
            // if (!logFile.is_open()) cerr << "Failed to open log file!" << endl;

            ofstream logFile("peer_network.txt", ios::app);
            if (!logFile) {
                cerr << "[ERROR] Failed to open peer_network.txt" << endl;
                return;
            }

            // logFile << time(0) << " - " << message << '\n';
            auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
            logFile << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S") << " - " << message << '\n';
            logFile.flush();

            cout << message << '\n';
        }

        void connectToPeer(const string& seedIP, int seedPort) {
            cout << "[DEBUG] Trying to connect to: " << seedIP << ":" << seedPort << endl;

            // Create a socket
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                cerr << "[ERROR] Socket creation failed!" << endl;
                return;
            }

            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(seedPort);
            inet_pton(AF_INET, seedIP.c_str(), &serverAddr.sin_addr);

            // Try connecting to the seed node
            if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                cerr << "[ERROR] Connection to " << seedIP << ":" << seedPort << " failed!" << endl;
                close(sock);
                return;
            }

            cout << "[DEBUG] Connection to " << seedIP << ":" << seedPort << " was successful." << endl;

            // Open log file in append mode
            ofstream logFile("peer_network.txt", ios::app);
            if (!logFile) {
                cerr << "[ERROR] Failed to open peer_network.txt" << endl;
                close(sock);
                return;
            }

            logFile << "[LOG] PeerNode connecting to " << seedIP << ":" << seedPort << endl;
            logFile.flush();  
            logFile.close();  

            close(sock);  // Close socket after logging
        }

        void connectToSeeds(){
            for (const auto& seed : connectedSeedAddr) {
                string selfAddress = myIpAddress + ":" + to_string(myPort);
                string seedStr;
                for (const auto& ch : seed) {
                    if (!std::isspace(ch)) seedStr += ch;
                }

                if(seedStr == selfAddress) continue;

                size_t pos = seed.find(":");
                if (pos != string::npos) {
                    string ip = seedStr.substr(0, pos);
                    int port = stoi(seedStr.substr(pos + 1));
                    cout << "[DEBUG] Connecting to seed: " << seed << '\n';
                    connectToPeer(ip, port);
                }
            }
        }
        
        void peerToPeerConnection(int clientSocket, string addr){
            char buffer[1024];
            while(1){
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                if(bytesReceived <= 0) break;
                string message(buffer);

                if(message.find("New Connect Request From") != string::npos){
                    send(clientSocket, "New Connect Accepted", 19, 0);
                    unique_lock<mutex> lock(mtx);
                    connectedPeers.push_back(addr);
                    logMessage("Connected to: " + addr);
                }

                else{
                    logMessage("Received: " + message + " from " + addr);
                    if(messageList.find(message) == messageList.end()){
                        messageList.insert(message);
                        for(const auto& peer : connectedPeers){
                            size_t pos = peer.find(":");
                            if (pos != string::npos) {
                                string ip = peer.substr(0, pos);
                                int port = stoi(peer.substr(pos + 1));
                                connectToPeer(ip, port);
                            }
                        }
                    }
                }
            }

            close(clientSocket);
        }

        void startServer(){
            int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in serverAddr{};
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = inet_addr(myIpAddress.c_str());
            serverAddr.sin_port = htons(myPort);
            bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
            listen(serverSocket, 5);

            while(1){
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
                string clientIp = inet_ntoa(clientAddr.sin_addr);
                thread(&PeerNode::peerToPeerConnection, this, clientSocket, clientIp).detach();
            }

            close(serverSocket);
        }

        void registerWithKSeeds() {
            vector<string> seeds(seedsAddresses.begin(), seedsAddresses.end());
            int numSeedsToConnect = (seeds.size() / 2) + 1;
            random_shuffle(seeds.begin(), seeds.end());
            connectedSeedAddr.assign(seeds.begin(), seeds.begin() + numSeedsToConnect);
            connectToSeeds();
        }

        void executeJob() {
            while(1){
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this] {return !jobQueue.empty(); });
                int job = jobQueue.front();
                jobQueue.pop();
                lock.unlock();

                if(job == 1) startServer();
            }
        }

        void run() {
            ifstream configFile("config.txt");
            if (!configFile) {
                cerr << "[ERROR] Failed to open config.txt" << endl;
                return;
            }

            string seed;
            while (getline(configFile, seed)) {
                if (!seed.empty()) {
                    cout << "[DEBUG] Read seed address: " << seed << endl;
                    seedsAddresses.insert(seed);
                }
            }
            configFile.close();
    
            registerWithKSeeds();

            vector<thread> threads;
            for (int i = 0; i < 3; ++i) threads.emplace_back(&PeerNode::executeJob, this);

            for (int i = 1; i <= 3; ++i) {
                jobQueue.push(i);
                cv.notify_one();
            }

            for (auto &t : threads) t.join();
        }

};