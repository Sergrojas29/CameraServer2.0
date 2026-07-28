#pragma once
#include "CameraClient.h"
#include "CameraManager.h"
#include <vector>

class CameraManager {
private:
    int TotalPhotoCount;
    
    std::vector<CameraClient> connectedCameras;



public:
  CameraManager(){};
  ~CameraManager(){};

  CameraManager(CameraManager &&) noexcept = default;
  CameraManager &operator=(CameraManager &&) noexcept = default;

  CameraManager(const CameraManager &) = delete;
  CameraManager &operator=(const CameraManager &) = delete;

  /**
 * @brief Go through all available connected Cameras and Create a List of Camera Clients 
 *
 * @return void return std::error
 * 
 * @throws std::runtime_error If the internal synchronization primitive fails.
 */
  void ConnnectMultiCamera();
  void MultiCameraCapture();

  std::vector<std::string> Get_allImagePaths();
};
