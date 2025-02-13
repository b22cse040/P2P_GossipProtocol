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
        PeerNode(int port) : myPort(port), logFile("peer_network.log", ios::app) {};

        void logMessage(const string& message){
            lock_guard<mutex> lock(mtx);
            logFile << time(0) << " - " << message << '\n';
            cout << message << '\n';
        }

        void connectToPeer(const string &peerAddress, const string& message = ""){
            int peerSocket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in peerAddr{};
            peerAddr.sin_family = AF_INET;
            size_t pos = peerAddress.find(":");
            peerAddr.sin_addr.s_addr = inet_addr(peerAddress.substr(0, pos).c_str());
            peerAddr.sin_port = htons(stoi(peerAddress.substr(pos + 1)));

            if(connect(peerSocket, (struct sockaddr *)&peerAddr, sizeof(peerAddr) == 0)){
                string msg = (message.empty()) ? "New Connect Request From:" + myIpAddress + ":" + to_string(myPort) : message;
                send(peerSocket, msg.c_str(), msg.size(), 0);
                char buffer[1024] = {0};
                recv(peerSocket, buffer, 1024, 0);
                logMessage("Sent to " + peerAddress + ": " + msg);
                close(peerSocket);
            }
        }

        void connectToSeeds(){
            for(const auto& seed : connectedSeedAddr) connectToPeer(seed);
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
                            if(peer != addr) connectToPeer(peer, message);
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

        void run(){
            ifstream configFile("config.txt");
            string seed;
            while(getline(configFile, seed)){
                if(!seed.empty()) seedsAddresses.insert("172.31.74.146" + seed.substr(seed.find(":") + 1));
            }
            configFile.close();

            registerWithKSeeds();

            vector<thread> threads;
            for(int i = 0; i < 3; ++i) threads.emplace_back(&PeerNode::executeJob, this);

            for(int i = 1; i <= 3; ++i){
                jobQueue.push(i);
                cv.notify_one();
            }

            for(auto &t : threads) t.join();
        }
};