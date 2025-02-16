// #include <bits/stdc++.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <fstream>
// #include <sstream>
// #include <mutex>
// #include <thread>
// #include <condition_variable>
// using namespace std;

// class PeerNode {
//     private:
//         mutex mtx;
//         condition_variable cv;
//         queue<int> jobQueue;
//         set<string> seedsAddresses;
//         vector<string> connectedPeers;
//         vector<string> connectedSeedAddr;
//         unordered_set<string> messageList;
//         string myIpAddress = "172.31.69.51";
//         int myPort;
//         ofstream logFile;
//         unordered_map<string, int> peerFailureCount;

//     public:
//     void reportDeadPeer(const string& peer) {
//     for (const auto& seed : connectedSeedAddr) {
//         int seedPort = stoi(seed.substr(seed.find(":") + 1));

//         int sock = socket(AF_INET, SOCK_STREAM, 0);
//         sockaddr_in seedAddr{};
//         seedAddr.sin_family = AF_INET;
//         seedAddr.sin_port = htons(seedPort);
//         inet_pton(AF_INET, seed.substr(0, seed.find(":")).c_str(), &seedAddr.sin_addr);

//         if (connect(sock, (struct sockaddr*)&seedAddr, sizeof(seedAddr)) < 0) continue;

//         string deadMessage = "Dead Node:" + peer;
//         send(sock, deadMessage.c_str(), deadMessage.size(), 0);
//         close(sock);
//     }
// }

// void pingPeers() {
//     while (true) {
//         this_thread::sleep_for(chrono::seconds(13));
//         for (const auto& peer : connectedPeers) {
//             string pingCommand = "ping -c 1 " + peer.substr(0, peer.find(":"));
//             if (system(pingCommand.c_str()) != 0) {
//                 reportDeadPeer(peer);
//             }
//         }
//     }
// }

//         PeerNode(int port) : myPort(port) {
//             ofstream logFile("peer_network.txt", ios::app);
//             if (!logFile) {
//                 cerr << "[ERROR] Failed to open peer_network.txt" << endl;
//                 return;
//             }
//             logFile << "[LOG] PeerNode initialized at port " << port << endl;
//             logFile.close();
//         }

//         void logMessage(const string& message){
//             // lock_guard<mutex> lock(mtx);
//             // if (!logFile.is_open()) cerr << "Failed to open log file!" << endl;

//             ofstream logFile("peer_network.txt", ios::app);
//             if (!logFile) {
//                 cerr << "[ERROR] Failed to open peer_network.txt" << endl;
//                 return;
//             }

//             // logFile << time(0) << " - " << message << '\n';
//             auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
//             logFile << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S") << " - " << message << '\n';
//             logFile.flush();

//             cout << message << '\n';
//         }

//         void connectToPeer(const string& seedIP, int seedPort) {
//             cout << "[DEBUG] Trying to connect to: " << seedIP << ":" << seedPort << endl;

//             // Create a socket
//             int sock = socket(AF_INET, SOCK_STREAM, 0);
//             if (sock < 0) {
//                 cerr << "[ERROR] Socket creation failed!" << endl;
//                 return;
//             }

//             struct sockaddr_in serverAddr;
//             serverAddr.sin_family = AF_INET;
//             serverAddr.sin_port = htons(seedPort);
//             inet_pton(AF_INET, seedIP.c_str(), &serverAddr.sin_addr);

//             // Try connecting to the seed node
//             if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//                 cerr << "[ERROR] Connection to " << seedIP << ":" << seedPort << " failed!" << endl;
//                 close(sock);
//                 return;
//             }

//             cout << "[DEBUG] Connection to " << seedIP << ":" << seedPort << " was successful." << endl;

//             // Open log file in append mode
//             ofstream logFile("peer_network.txt", ios::app);
//             if (!logFile) {
//                 cerr << "[ERROR] Failed to open peer_network.txt" << endl;
//                 close(sock);
//                 return;
//             }

//             logFile << "[LOG] PeerNode connecting to " << seedIP << ":" << seedPort << endl;
//             logFile.flush();
//             logFile.close();

//             close(sock);  // Close socket after logging
//         }

//         void connectToSeeds(){
//             for (const auto& seed : connectedSeedAddr) {
//                 string selfAddress = myIpAddress + ":" + to_string(myPort);
//                 string seedStr;
//                 for (const auto& ch : seed) {
//                     if (!std::isspace(ch)) seedStr += ch;
//                 }

//                 if(seedStr == selfAddress) continue;

//                 size_t pos = seed.find(":");
//                 if (pos != string::npos) {
//                     string ip = seedStr.substr(0, pos);
//                     int port = stoi(seedStr.substr(pos + 1));
//                     cout << "[DEBUG] Connecting to seed: " << seed << '\n';
//                     connectToPeer(ip, port);
//                 }
//             }
//         }

//         void peerToPeerConnection(int clientSocket, string addr){
//             char buffer[1024];
//             while(1){
//                 memset(buffer, 0, sizeof(buffer));
//                 int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
//                 if(bytesReceived <= 0) break;
//                 string message(buffer);

//                 if(message.find("New Connect Request From") != string::npos){
//                     send(clientSocket, "New Connect Accepted", 19, 0);
//                     unique_lock<mutex> lock(mtx);
//                     connectedPeers.push_back(addr);
//                     logMessage("Connected to: " + addr);
//                 }

//                 else{
//                     logMessage("Received: " + message + " from " + addr);
//                     if(messageList.find(message) == messageList.end()){
//                         messageList.insert(message);
//                         for(const auto& peer : connectedPeers){
//                             size_t pos = peer.find(":");
//                             if (pos != string::npos) {
//                                 string ip = peer.substr(0, pos);
//                                 int port = stoi(peer.substr(pos + 1));
//                                 connectToPeer(ip, port);
//                             }
//                         }
//                     }
//                 }
//             }

//             close(clientSocket);
//         }

//         void startServer(){
//             int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//             sockaddr_in serverAddr{};
//             serverAddr.sin_family = AF_INET;
//             serverAddr.sin_addr.s_addr = inet_addr(myIpAddress.c_str());
//             serverAddr.sin_port = htons(myPort);
//             bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
//             listen(serverSocket, 5);

//             while(1){
//                 sockaddr_in clientAddr;
//                 socklen_t clientLen = sizeof(clientAddr);
//                 int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
//                 string clientIp = inet_ntoa(clientAddr.sin_addr);
//                 thread(&PeerNode::peerToPeerConnection, this, clientSocket, clientIp).detach();
//             }

//             close(serverSocket);
//         }

//         void registerWithKSeeds() {
//             vector<string> seeds(seedsAddresses.begin(), seedsAddresses.end());
//             int numSeedsToConnect = (seeds.size() / 2) + 1;
//             random_shuffle(seeds.begin(), seeds.end());
//             connectedSeedAddr.assign(seeds.begin(), seeds.begin() + numSeedsToConnect);
//             connectToSeeds();
//         }

//         void executeJob() {
//             while(1){
//                 unique_lock<mutex> lock(mtx);
//                 cv.wait(lock, [this] {return !jobQueue.empty(); });
//                 int job = jobQueue.front();
//                 jobQueue.pop();
//                 lock.unlock();

//                 if(job == 1) startServer();
//             }
//         }

//         void run() {
//             ifstream configFile("config.txt");
//             if (!configFile) {
//                 cerr << "[ERROR] Failed to open config.txt" << endl;
//                 return;
//             }

//             string seed;
//             while (getline(configFile, seed)) {
//                 if (!seed.empty()) {
//                     cout << "[DEBUG] Read seed address: " << seed << endl;
//                     seedsAddresses.insert(seed);
//                 }
//             }
//             configFile.close();

//             registerWithKSeeds();

//             vector<thread> threads;
//             for (int i = 0; i < 3; ++i) threads.emplace_back(&PeerNode::executeJob, this);

//             for (int i = 1; i <= 3; ++i) {
//                 jobQueue.push(i);
//                 cv.notify_one();
//             }

//             for (auto &t : threads) t.join();
//         }

// };
#include <bits/stdc++.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>

using namespace std;

class PeerNode
{
private:
    mutex mtx;
    condition_variable cv;
    set<string> seedsAddresses;
    vector<string> connectedPeers;
    unordered_map<string, int> peerFailureCount;
    string myIpAddress = "127.0.0.1";
    int myPort;
    ofstream logFile;

public:
    vector<string> getConnectedPeers() {
        lock_guard<mutex> lock(mtx);
        return connectedPeers;
    }
    PeerNode(int port) : myPort(port) {
            ofstream logFile("peer_network.txt", ios::app);
            if (!logFile) {
                cerr << "[ERROR] Failed to open peer_network.txt" << endl;
                return;
            }
            logFile << "[LOG] PeerNode initialized at port " << port << endl;
            logFile.close();
        }

    void logMessage(const string &message)
    {
        lock_guard<mutex> lock(mtx);
        if (!logFile.is_open())
            return;
        logFile << message << endl;
        cout << message << endl;
    }

    void readConfig(const string &filename)
    {
        ifstream file(filename);
        if (!file)
            return;
        string line;
        while (getline(file, line))
            seedsAddresses.insert(line);
    }

    void registerWithSeeds()
    {
        readConfig("config.txt");
        // int minConnections = max(1, (int)seedsAddresses.size() / 2);
        // vector<string> seedList(seedsAddresses.begin(), seedsAddresses.end());
        // shuffle(seedList.begin(), seedList.end(), default_random_engine(random_device()()));
        int minConnections = 1;
        vector<string> seedList(seedsAddresses.begin(), seedsAddresses.end());
        cout<<"mininmunm cinnec"<<minConnections<<endl;
        cout<<seedList[0]<<endl;
        for (int i = 0; i < minConnections; ++i)
        {
            string seedIp = seedList[i].substr(0, seedList[i].find(":"));
            int seedPort = stoi(seedList[i].substr(seedList[i].find(":") + 1));

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in seedAddr{};
            seedAddr.sin_family = AF_INET;
            seedAddr.sin_port = htons(seedPort);
            inet_pton(AF_INET, seedIp.c_str(), &seedAddr.sin_addr);

            if (connect(sock, (struct sockaddr *)&seedAddr, sizeof(seedAddr)) < 0)
                continue;
            else
                cout<<"connected"<<endl;

            string registerMessage = "Register:" + to_string(myPort);
            send(sock, registerMessage.c_str(), registerMessage.size(), 0);

            cout<<"THere"<<endl;
            char buffer[1024] = {0};
            int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
            cout<<"Here"<<endl;
            if (bytesReceived > 0)
                parseAndConnectPeers(string(buffer));

            // close(sock);
        }

        cout<<"exit"<<endl;
    }

    void parseAndConnectPeers(const string &peerList)
    {
        istringstream ss(peerList);
        string peer;
        cout<<peerList<<endl;
        while (getline(ss, peer, ','))
        {
            if (!peer.empty() && peer != myIpAddress + ":" + to_string(myPort))
            {
                connectedPeers.push_back(peer);
                connectToPeer(peer.substr(0, peer.find(":")), stoi(peer.substr(peer.find(":") + 1)), "Hello Peer");
            }
        }
    }

    void sendGossipMessage()
    {
        while (true)
        {
            this_thread::sleep_for(chrono::seconds(5));
            string message = to_string(time(nullptr)) + ":" + myIpAddress + ":" + to_string(myPort);

            for (const auto &peer : connectedPeers)
            {
                connectToPeer(peer.substr(0, peer.find(":")), stoi(peer.substr(peer.find(":") + 1)), message);
            }
        }
    }

    void pingPeers()
    {
        while (true)
        {
            this_thread::sleep_for(chrono::seconds(13));
            for (auto &peer : connectedPeers)
            {
                string ip = peer.substr(0, peer.find(":"));
                string pingCommand = "ping -c 1 " + ip;
                if (system(pingCommand.c_str()) != 0)
                {
                    peerFailureCount[peer]++;
                    if (peerFailureCount[peer] >= 3)
                    {
                        reportDeadPeer(peer);
                        connectedPeers.erase(remove(connectedPeers.begin(), connectedPeers.end(), peer), connectedPeers.end());
                    }
                }
                else
                    peerFailureCount[peer] = 0;
            }
        }
    }

    void reportDeadPeer(const string &peer)
    {
        for (const auto &seed : seedsAddresses)
        {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in seedAddr{};
            seedAddr.sin_family = AF_INET;
            seedAddr.sin_port = htons(stoi(seed.substr(seed.find(":") + 1)));
            inet_pton(AF_INET, seed.substr(0, seed.find(":")).c_str(), &seedAddr.sin_addr);

            if (connect(sock, (struct sockaddr *)&seedAddr, sizeof(seedAddr)) < 0)
                continue;
            string deadMessage = "Dead Node:" + peer;
            send(sock, deadMessage.c_str(), deadMessage.size(), 0);
            close(sock);
        }
    }

   void connectToPeer(string peerIp, int peerPort, string message) {
    printf("[DEBUG] Attempting to connect to %s:%d\n", peerIp.c_str(), peerPort);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "[ERROR] Failed to create socket!" << endl;
        return;
    }
    printf("[DEBUG] Socket created successfully\n");
    printf("%d",myPort);
   
    sockaddr_in peerAddr{};
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(peerPort);
    inet_pton(AF_INET, peerIp.c_str(), &peerAddr.sin_addr);

    printf("[DEBUG] Trying to connect to %s:%d\n", peerIp.c_str(), peerPort);

    if (connect(sock, (struct sockaddr*)&peerAddr, sizeof(peerAddr)) < 0) {
        perror("[ERROR] Connect failed");
        close(sock);
        return;
    }
    printf("[DEBUG] Connected to peer %s:%d\n", peerIp.c_str(), peerPort);

    printf("[DEBUG] Successfully connected to peer %s:%d\n", peerIp.c_str(), peerPort);
    send(sock, message.c_str(), message.size(), 0);
    printf("[DEBUG] Message sent: %s\n", message.c_str());

    close(sock);
    printf("[DEBUG] Socket closed\n");
}

    void run()
    {
        logMessage("[INFO] PeerNode started at " + myIpAddress + ":" + to_string(myPort));

        // Create a listening socket
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0)
        {
            cerr << "[ERROR] Failed to create server socket" << endl;
            return;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(myPort);
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            cerr << "[ERROR] Failed to bind server socket on port " << myPort << endl;
            return;
        }

        if (listen(serverSocket, 5) < 0)
        {
            cerr << "[ERROR] Failed to listen on port " << myPort << endl;
            return;
        }

        logMessage("[INFO] PeerNode is now LISTENING on port " + to_string(myPort));

        thread gossipThread(&PeerNode::sendGossipMessage, this);
        thread pingThread(&PeerNode::pingPeers, this);

        gossipThread.join();
        pingThread.join();

        close(serverSocket);
    }
};
