#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread> // Included for multithreading
#include "store.h"
#include <csignal>

// Global pointer for the signal handler to access the store on exit
Store* global_store = nullptr;

void handle_signal(int signal) {
    if (global_store) {
        global_store->save_to_disk("miniredis_dump.txt");
    }
    std::exit(0);
}

// Helper function to split the input line into tokens
std::vector<std::string> parse_command(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// The function that each individual thread will run
void handle_client(int client_fd, Store& store) {
    std::cout << "Thread ID " << std::this_thread::get_id() << " handling new client.\n";
    
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer)); 
        
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            std::cout << "Client disconnected from thread " << std::this_thread::get_id() << ".\n";
            break; 
        }

        std::string line(buffer);
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (line.empty()) continue;

        std::vector<std::string> args = parse_command(line);
        std::string cmd = args[0];
        for (char &c : cmd) c = toupper(c);

        std::string response;

        if (cmd == "QUIT" || cmd == "EXIT") {
            break;
        } else if (cmd == "SET" && args.size() == 3) {
            store.set(args[1], args[2]);
            response = "+OK\n";
        } else if (cmd == "GET" && args.size() == 2) {
            response = store.get(args[1]) + "\n";
        } else if (cmd == "DEL" && args.size() == 2) {
            bool deleted = store.del(args[1]);
            response = deleted ? "(integer) 1\n" : "(integer) 0\n";
        } else if (cmd == "EXISTS" && args.size() == 2) {
            bool exists = store.exists(args[1]);
            response = exists ? "(integer) 1\n" : "(integer) 0\n";
        } else if (cmd == "EXPIRE" && args.size() == 3) {
            try {
                int seconds = std::stoi(args[2]);
                bool success = store.expire(args[1], seconds);
                response = success ? "(integer) 1\n" : "(integer) 0\n";
            } catch (const std::exception&) {
                response = "-ERR value is not an integer or out of range\n";
            }
        } else if (cmd == "TTL" && args.size() == 2) {
            int ttl_val = store.ttl(args[1]);
            response = "(integer) " + std::to_string(ttl_val) + "\n";
        } else if (cmd == "KEYS" && args.size() == 1) {
            std::vector<std::string> keys = store.keys();
            if (keys.empty()) {
                response = "(empty array)\n";
            } else {
                for (size_t i = 0; i < keys.size(); ++i) {
                    response += std::to_string(i + 1) + ") \"" + keys[i] + "\"\n";
                }
            }
        } else {
            response = "-ERR unknown command or wrong number of arguments\n";
        }

        send(client_fd, response.c_str(), response.length(), 0);
    }
    
    close(client_fd);
}

int main() {
    Store store;
    global_store = &store;

    // Register signal handlers for both Ctrl+C (SIGINT) and Docker stop (SIGTERM)
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // Automatically load data from disk if a snapshot exists
    store.load_from_disk("miniredis_dump.txt");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // This option allows us to restart the server immediately without waiting for the port to time out
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(6380); 

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "MiniRedis Server listening on port 6380...\n";
    
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Accept failed\n";
            continue; 
        }

        // Spawn a new thread per client connection
        std::thread client_thread(handle_client, client_fd, std::ref(store));
        
        // Detach the thread so it runs independently in the background
        client_thread.detach(); 
    }

    close(server_fd);
    return 0;
}
