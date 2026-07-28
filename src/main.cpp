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
#include <vector>

#include "CameraManager.h"
#include "crow/json.h"
#include "nlohmann/json_fwd.hpp"

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

void readTerminal(CameraManager &Manager) {
  while (true) {
    std::string input;
    std::getline(std::cin, input);

    if (input == "quit") {
      return;
    } else if (input == "PHOTO") {
      Manager.MultiCameraCapture();
    } else if (input == "PATH") {
      std::vector<std::string> paths = Manager.Get_allImagePaths();
      crow::json::wvalue data;

      data["action"] = "ImagePath";
      data["count"] = paths.size();
      data["image_paths"] = paths;

      std::lock_guard<std::mutex> lock(clients_mutex);
      for (auto &&client : active_connections) {
        client->send_text(data.dump());
      }
    } else {
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
        .onclose([&](crow::websocket::connection &conn,
                     const std::string &reason, uint16_t /*code*/) {
          CROW_LOG_INFO << "Websocket disconnected: " << reason;
          std::lock_guard<std::mutex> lock(clients_mutex);
          active_connections.erase(&conn);
        })
        .onmessage([&](crow::websocket::connection &conn,
                       const std::string &data, bool is_binary) {
          if (is_binary)
            return;

          auto message = crow::json::load(data);

          if (!message) {
            conn.send_text("{\"error\": \"Invalid JSON format\"}");
            return;
          }

          std::string APIKEY = "f19e946fe179e9fb2da37e8ec4126157";
          std::string url = "https://api.imgbb.com/1/upload";

          std::vector<std::string> uploadPath;

          for (auto &item : message) {
            try {

              std::string file_path = item["src"].s();

              std::print("{}", file_path);

              cpr::Response res = cpr::Post(
                  cpr::Url{url},
                  cpr::Multipart{
                      {"key", APIKEY},
                      // Replace with the path to the image you want to test
                      {"image", cpr::File{file_path}}});

              if (res.error || res.status_code != 200) {
                std::cout << "Response text: " << res.text << std::endl;
                throw std::runtime_error("Error uploading Image");
              }

              auto data = crow::json::load(res.text);
              if (!data) {
                throw std::runtime_error("API did not return valid JSON");
              }

              // 3. Safely check that the keys exist before calling .s()
              if (data["data"].has("display_url") && data["data"].has("url")) {
                std::string url_viewer = data["data"]["display_url"].s();
                std::string url_full_size = data["data"]["url"].s();

                url_viewer.insert(0, "\"");
                url_viewer += "\"";

                uploadPath.push_back(url_viewer);
              } else {
                throw std::runtime_error(
                    "API JSON was missing expected URL keys");
              }

            } catch (const std::exception &e) {
              std::cerr << "Exception caught: " << e.what() << std::endl;
            }
          }

          std::string display_Url =
              "https://sergrojas29.github.io/Image_dispaly/?images=[";

          std::string combined_images = "";
          for (size_t i = 0; i < uploadPath.size(); ++i) {
            combined_images += uploadPath[i];
            if (i < uploadPath.size() - 1) {
              combined_images += ",";
            }
          }

          combined_images += "]";
          display_Url += combined_images;

          std::println("\n Http url : {}", display_Url);
        });

    app.port(18080).multithreaded().run();

    input_thread.join();

    return 1;
  }