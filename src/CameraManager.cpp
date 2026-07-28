#include "CameraManager.h"
#include <print>
#include <string>
#include <vector>

void check_gphoto_error(int rc, const std::string &context_msg) {
  if (rc < GP_OK) {
    throw std::runtime_error(context_msg +
                             " (Error code: " + std::to_string(rc) + ")");
  }
}

void CameraManager::ConnnectMultiCamera() {
  kill_auto_mount();

  GPContext *context = gp_context_new();
  std::unique_ptr<GPContext, ContextDeleter> context_ptr(context);

  CameraList *cameraList;
  gp_list_new(&cameraList);
  std::unique_ptr<CameraList, CameraListDeleter> camera_list_ptr(cameraList);

  gp_camera_autodetect(camera_list_ptr.get(), context_ptr.get());

  int count = gp_list_count(camera_list_ptr.get());
  std::println("Found {} camara(s)", count);

  if (count < 1)
    return;

  // CREATE LIST OF CAMERAS
  GPPortInfoList *portInfoList;
  gp_port_info_list_new(&portInfoList);
  gp_port_info_list_load(portInfoList);
  std::unique_ptr<GPPortInfoList, GPPortInfoListDeleter> portInfoList_ptr(
      portInfoList);

  CameraAbilitiesList *abilitiesList;
  gp_abilities_list_new(&abilitiesList);
  gp_abilities_list_load(abilitiesList, context_ptr.get());
  std::unique_ptr<CameraAbilitiesList, CameraAbilitiesListDeleter>
      abilitiesList_ptr(abilitiesList);

  for (int i = 0; i < count; ++i) {
    const char *name, *port;
    gp_list_get_name(camera_list_ptr.get(), i, &name);
    gp_list_get_value(camera_list_ptr.get(), i, &port);

    std::println("\nAttempting to connect to [{}]:  {} on ", i, name);

    try {
      // A. Get camera model abilities
      int modelIdx =
          gp_abilities_list_lookup_model(abilitiesList_ptr.get(), name);
      check_gphoto_error(modelIdx, "Failed to lookup model");
      CameraAbilities abilities;
      gp_abilities_list_get_abilities(abilitiesList_ptr.get(), modelIdx,
                                      &abilities);

      // B. Get specific port info
      int portIdx = gp_port_info_list_lookup_path(portInfoList_ptr.get(), port);
      check_gphoto_error(portIdx, "Failed to lookup port");
      GPPortInfo portInfo;
      gp_port_info_list_get_info(portInfoList_ptr.get(), portIdx, &portInfo);

      CameraClient NewCamera(abilities, portInfo, name);
      NewCamera.connect();

      connectedCameras.push_back(std::move(NewCamera));

      std::println("Successfully connected to {} ", name);

    } catch (const std::exception &e) {
      std::println("Error configuring camera {} : {} ", i, e.what());
    }
  }
};

void CameraManager::MultiCameraCapture() {

#pragma omp parallel for
  for (int i = 0; i < connectedCameras.size(); i++) {
    auto &cam = connectedCameras[i];
    //Set file path name
    std::string filePath = "public/" + cam.camera_name + '_' +
                           std::to_string(cam.m_photo_count) + ".jpg";
    //Take Photo
    cam.capturePhoto(filePath);

    // Add File Path to Camera Client
    cam.Set_image_path(filePath);
  };
};


std::vector<std::string> CameraManager::Get_allImagePaths(){
    std::vector<std::string> all_photo_paths;

    for( auto&& cam : connectedCameras){
      std::vector<std::string> newVec = cam.Get_allImagePaths();
      all_photo_paths.insert(all_photo_paths.end(), newVec.begin(), newVec.end());
    }

    return all_photo_paths;
};