#include "CameraClient.h"
#include <algorithm>
#include <chrono>
#include <print>
#include <stdexcept>
#include <string>

// FUNC Help Functions
void kill_auto_mount() {
  system("pkill -f gphoto");
  system("pkill -f gvfs-gphoto2");
  system("sleep 1");
}

// FUNC PUBLIC MEMBER
CameraClient::CameraClient(CameraAbilities& abilities, GPPortInfo& portInfo, std::string cam_name) : m_context(gp_context_new()) {
  Camera *raw_cam = nullptr;
  gp_camera_new(&raw_cam);
  gp_camera_set_abilities(raw_cam,abilities);
  gp_camera_set_port_info(raw_cam, portInfo);
  m_camera.reset(raw_cam);

  camera_name = cam_name;

}

CameraClient::~CameraClient() {
  if (m_camera && m_context) {
    gp_camera_exit(m_camera.get(), m_context.get());
  }
}


bool CameraClient::connect() {
  if (isConnected()) {
    return true;
  } else {
    kill_auto_mount();
    std::println("Connection Attempt ... ");

    int ret = gp_camera_init(m_camera.get(), m_context.get());
    if (ret < GP_OK) {
      std::println("Connection Error: {}" ,ret);
      return false;
    }
    std::println("Connected Successfully!");
    m_connected = true;
    return true;
  }
}

bool CameraClient::capturePhoto(std::string SaveImagePath) {
  try {


    CameraFilePath camera_file_path;
    // pointer to camera, Opt , Filepath, pointer to context
    int ret = gp_camera_capture(m_camera.get(), GP_CAPTURE_IMAGE,
                                &camera_file_path, m_context.get());

    if (ret < GP_OK) {
      std::string err = "Camera Capture Fail Error: " + std::to_string(ret);
      throw std::runtime_error(err);
    }

    std::cout << "Image capture on camera: " << camera_file_path.name
              << std::endl;

    // 5.Download
    std::cout << "Downloading Image ..." << std::endl;

    // Init Camera_File Raw pointer
    CameraFile *raw_file = nullptr;
    gp_file_new(&raw_file);
    std::unique_ptr<CameraFile, FileDeleter> file(raw_file);

    ret = gp_camera_file_get(m_camera.get(), camera_file_path.folder,
                             camera_file_path.name, GP_FILE_TYPE_NORMAL,
                             file.get(), m_context.get());

    if (ret < GP_OK)
      throw std::runtime_error("Could NOT download image");

    gp_file_save(file.get(), SaveImagePath.c_str());
    std::cout << "Success! saved to " << SaveImagePath << std::endl;

    //Increase Photo count
    m_photo_count++;
  
    return true;

  } catch (const std::exception &e) {
    std::cerr << "Exception caught: " << e.what() << std::endl;
    return false;
  }
};




