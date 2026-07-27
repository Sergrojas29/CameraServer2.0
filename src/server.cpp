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

#include "CameraClient.h"



std::unordered_map<std::string, std::function<void(std::string)>> message_handler = {
    {"IMAGES", [](std::string message) {
        std::print("IMAGES WAS CALLED: {}\n", message);
    }},
    {"TEXT", [](std::string message) {
        std::print("TEXT WAS CALLED: {}\n", message);
    }}
};

std::unordered_set<crow::websocket::connection *> active_connections;
std::mutex clients_mutex;

uint photo_count = 0;
std::mutex photo_count_mutex;

void readTerminal(CameraClient &Cam1, CameraClient &Cam2 ) {
  while (true) {
    std::string input;
    std::getline(std::cin, input);

    if (input == "quit") {
      return;
    } else if (input == "PHOTO") {
      std::lock_guard<std::mutex> lock(photo_count_mutex);
      Cam1.capturePhoto("photos/img_" + std::to_string(photo_count)+".jpeg");
      
      ++photo_count;
      Cam2.capturePhoto("photos/img_" + std::to_string(photo_count)+".jpeg");
    }
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto &&client : active_connections) {
      client->send_text(input);
    }
  }
}

int main() {
  crow::SimpleApp app;
  CameraClient Cam1;
  CameraClient Cam2;

  
  std::thread input_thread(readTerminal, std::ref(Cam1), std::ref(Cam1));

  Cam1.connect();
  Cam2.connect();

  input_thread.join();

  return 1;
}