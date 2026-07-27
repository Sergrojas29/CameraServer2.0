#pragma once
#include <gphoto2-camera.h>
#include <gphoto2-list.h>
#include <gphoto2/gphoto2.h>
#include "nlohmann/json.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cpr/cpr.h>
#include <cstddef>
#include <cstdlib>
#include <cups/cups.h>
// #include <execution>
#include <filesystem>
#include <format>
#include <future>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// helper to tell unique_ptr how to free a Camera
struct CameraDeleter {
  void operator()(Camera *p) const {
    // We only free if p is not null
    if (p) {
      gp_camera_free(p);
    }
  }
};

//Helper to delete Camera List
struct CameraListDeleter{
  void operator()(CameraList *p)const{
    if(p){
      gp_list_free(p);
    }
  }
};

// helper to tell unique_ptr how to free a Context
struct ContextDeleter {
  void operator()(GPContext *p) const {
    if (p)
      gp_context_unref(p);
  }
};

struct FileDeleter {
  void operator()(CameraFile *p) const {
    if (p)
      gp_file_free(p); // Free the file memory
  }
};


struct GPPortInfoListDeleter{
  void operator()(GPPortInfoList *p) const{
    if(p){
      gp_port_info_list_free(p);
    }
  }
};

struct CameraAbilitiesListDeleter{
  void operator()(CameraAbilitiesList *p) const{
    if(p){
      gp_abilities_list_free(p);
    }
  }
};

