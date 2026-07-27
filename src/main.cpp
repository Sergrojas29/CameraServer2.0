#include "CameraClient.h"
#include "UniqueCamPtr.h"
#include <algorithm>
#include <cstdio>
#include <gphoto2-list.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <iostream>
#include <memory>
#include <print>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
// Helper to throw C++ exceptions on gphoto2 errors
void check_gphoto_error(int rc, const std::string &context_msg) {
  if (rc < GP_OK) {
    throw std::runtime_error(context_msg +
                             " (Error code: " + std::to_string(rc) + ")");
  }
}

int main() {

  //**
  // GET HOW MANY CAMERAS ARE CONNECTED
  //  */

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
    return 1;

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


    std::vector<CameraClient> connectedCameras;

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

      CameraClient NewCamera(abilities,portInfo);
      NewCamera.connect();


      connectedCameras.push_back(std::move(NewCamera));

      std::println("Successfully connected to {} ", name);

    } catch (const std::exception &e) {
      std::println("Error configuring camera {} : {} ", i, e.what());
    }
  }

  int pcount = 0;
  for( auto&& cam: connectedCameras){
    cam.capturePhoto("image_"+ std::to_string(pcount)+ ".jpg");
    ++pcount;
  }

  return 1;
}