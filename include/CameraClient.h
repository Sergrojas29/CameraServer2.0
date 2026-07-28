#pragma once
#include "UniqueCamPtr.h"
#include <string>

// FUNC Help Functions
void kill_auto_mount();

class CameraClient {
private:
  std::unique_ptr<GPContext, ContextDeleter> m_context;
  std::unique_ptr<Camera, CameraDeleter> m_camera;
  bool m_connected = false;

  std::vector<std::string> ImagesPaths;



public:
  std::string camera_name;
  int m_photo_count = 0;

  CameraClient(CameraAbilities &abilities, GPPortInfo &portInfo,
               std::string cam_name);
  ~CameraClient();

  //! Explicitly tell the compiler to generate the Move semantics
  CameraClient(CameraClient &&) noexcept = default;
  CameraClient &operator=(CameraClient &&) noexcept = default;

  //! Explicitly delete the Copy semantics
  CameraClient(const CameraClient &) = delete;
  CameraClient &operator=(const CameraClient &) = delete;

  std::string m_save_path;

  /**
   * @brief Create or Set File path
   * @param string of the fodler or existing folder
   * @return bool of success/fail
   */
  bool connect();
  bool isConnected() const { return m_connected; }

  bool capturePhoto(std::string SaveImagePath);

  inline std::vector<std::string> Get_allImagePaths(){
    return ImagesPaths;
  };

  inline void Set_image_path(std::string new_image_Path){
    ImagesPaths.push_back(new_image_Path);
  };
};