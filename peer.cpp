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
    unordered_map<string, bool> messageLog;
    string myIpAddress = "127.0.0.1";
    int myPort;
    ofstream logFile;
    int messageCount=0;

public:
string generateGossipMessage() {
        return to_string(time(nullptr)) + ":" + myIpAddress + ":" + to_string(myPort) + ":" + " Gossip"+to_string(messageCount);
    }

 
    void forwardGossipMessage(const string& message, const string& senderIp, int senderPort) {
        lock_guard<mutex> lock(mtx);

       
        if (messageLog.find(message) != messageLog.end()) {
            return;  
        }

       
        messageLog[message] = true;

        logMessage("[INFO] Received and forwarding: " + message);

      
        for (const auto& peer : connectedPeers) {
            size_t pos = peer.find(":");
            if (pos != string::npos) {
                string peerIp = peer.substr(0, pos);
                int peerPort = stoi(peer.substr(pos + 1));

                
                if (peerIp == senderIp && peerPort == senderPort) continue;

                connectToPeer(peerIp, peerPort, message);
            }
        }
    }

   
    void sendGossipMessage() {
        while (messageCount < 10) {  
            this_thread::sleep_for(chrono::seconds(5));
            string message = generateGossipMessage();
            messageLog[message] = true;  

            logMessage("[GOSSIP] Sending: " + message);
            cout<<"Gossip Message : "<<message<<endl;
            // Send to all peers
            for (const auto& peer : connectedPeers) {
                size_t pos = peer.find(":");
                if (pos != string::npos) {
                    string peerIp = peer.substr(0, pos);
                    int peerPort = stoi(peer.substr(pos + 1));
                    connectToPeer(peerIp, peerPort, message);
                }
            }
            messageCount++;
        }
    }

    //  Function to listen for incoming messages
    void listenForMessages(int serverSocket) {
        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket < 0) continue;

            char buffer[1024] = {0};
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                string receivedMessage(buffer);
                string senderIp = inet_ntoa(clientAddr.sin_addr);
                int senderPort = ntohs(clientAddr.sin_port);
                forwardGossipMessage(receivedMessage, senderIp, senderPort);
            }
            close(clientSocket);
        }
    }

    vector<string> getConnectedPeers() {
        lock_guard<mutex> lock(mtx);
        return connectedPeers;
    }
    PeerNode(int port) : myPort(port) {
            ofstream logFile("peer_network.txt", ios::app);
            if (!logFile) {
               
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
        int minConnections = max(1, (int)seedsAddresses.size() / 2);
        vector<string> seedList(seedsAddresses.begin(), seedsAddresses.end());
        shuffle(seedList.begin(), seedList.end(), default_random_engine(random_device()()));
       
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
            ofstream graphFile("graph.txt",ios::app);
             graphFile << "Seed Connection " << myPort<<" "<<seedPort << "\n";
             graphFile.close();
            char buffer[1024] = {0};
            int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
            cout<<"Here"<<endl;
            if (bytesReceived > 0)
                parseAndConnectPeers(string(buffer));

           
        }

        
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
            
            return;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(myPort);
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
           
            return;
        }

        if (listen(serverSocket, 5) < 0)
        {
           
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
