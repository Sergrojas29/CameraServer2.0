#include "crow.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <print>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "CameraManager.h"

std::unordered_map<std::string, std::function<void(std::string)>>
    message_handler = {{"IMAGES",
                        [](std::string message) {
                          std::print("IMAGES WAS CALLED: {}\n", message);
                        }},
                       {"TEXT", [](std::string message) {
                          std::print("TEXT WAS CALLED: {}\n", message);
                        }}};

std::unordered_set<crow::websocket::connection *> active_connections;
std::mutex clients_mutex;

uint photo_count = 0;
std::mutex photo_count_mutex;

void readTerminal( CameraManager& Manager) {
  while (true) {
    std::string input;
    std::getline(std::cin, input);

    if (input == "quit") {
      return;
    }else if(input == "PHOTO"){
      Manager.MultiCameraCapture();
    }else if (input == "PATH") {
      std::vector<std::string> message = Manager.Get_allImagePaths();
      for (auto&& path : message) {
        std::println("{}", path);
      }
    }



    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto &&client : active_connections) {
      client->send_text(input);
    }
  }
}

int main() {
  crow::SimpleApp app;

  CameraManager Manager;

  Manager.ConnnectMultiCamera();


  std::thread input_thread(readTerminal, std::ref(Manager));

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onopen([&](crow::websocket::connection &conn) {
        CROW_LOG_INFO << "New websocket connection: " << &conn;
        std::lock_guard<std::mutex> lock(clients_mutex);
        active_connections.insert(&conn);
      })
      .onclose([&](crow::websocket::connection &conn, const std::string &reason,
                   uint16_t /*code*/) {
        CROW_LOG_INFO << "Websocket disconnected: " << reason;
        std::lock_guard<std::mutex> lock(clients_mutex);
        active_connections.erase(&conn);
      })
      .onmessage([&](crow::websocket::connection &conn, const std::string &data,
                     bool is_binary) {
        if (is_binary) {
          conn.send_binary(data);
        } else {
          conn.send_text("Echo: " + data);
          std::println("Echo: {}", data);
        }
      });

  


  input_thread.join();

  return 1;
}