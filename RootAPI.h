#pragma once
#include "./lib/API/Node.h"
#include "./lib/OSCAPI.h"
#include "GAPI.hpp"
#include "SPIFFS.h"
OSCAPI api;

struct RootAPI : public APIAndInstance<RootAPI>, public MapNode {
  RootAPI(GrideyeImpl &ge)
      : APIAndInstance<RootAPI>(*this), MapNode(), gapi(ge),
        spiffPrefix("/spiffs") {
    rGetSet<std::string>("state", &RootAPI::getState, &RootAPI::setState);
    rMember<bool>("sendMat", &RootAPI::sendMat);
    rFunction<std::string, std::string>("getFile", &RootAPI::getFile);
    rFunction<bool, std::string, std::string>("saveFile", &RootAPI::saveFile);
    rFunction<std::string>("ls", &RootAPI::ls);

    rTrig("reboot", &RootAPI::reboot);
    rTrig("reformat", &RootAPI::reformat);
    MapNode::addChild("g", &gapi);

    // MapNode::addChild("r", this); // false));
  }

  bool setup() {

    if (!SPIFFS.begin(true, spiffPrefix.c_str())) {
      return false;
    }
    PRINT("SPIFFS mounted , used ");
    PRINT(SPIFFS.usedBytes());
    PRINT(" / ");
    PRINTLN(SPIFFS.totalBytes());
    lsFrom("/");
    PRINTLN("with preffix");
    lsFrom(spiffPrefix);
    return true;
  }

  void reformat() {
    PRINTLN("reformatting....");
    SPIFFS.end();
    SPIFFS.format();
    setup();
  }

  void setState(const std::string &st) { gapi.fromString(st); }
  std::string getState() const {
    return ((APIInstanceBase *)&gapi)->toString();
  }

  bool saveFile(std::string path, std::string value) {
    path = spiffPrefix + path;
    File file = SPIFFS.open(&path[0], "w");
    if (!file) {
      PRINTLN("file not opened");
      return false;
    }
    file.println(value.c_str());
    file.flush();

    bool wroteAll = false;

    file.close();

    if (File file = SPIFFS.open(&path[0], "r")) {
      PRINT(file.name());
      PRINT(" : ");
      PRINT("wrote ");
      PRINTLN(file.size());
      PRINT(", from ");
      PRINTLN(value.size());
      wroteAll = file.size() >= value.size();

    } else {
      PRINTLN("write failed");
    }
    return wroteAll;
  }

  std::string ls() { return lsFrom(spiffPrefix); }
  std::string lsFrom(const std::string &from) {
    File root = SPIFFS.open(&from[0], "r");
    if (!root) {
      std::string err = "can't open : " + from;
      PRINTLN(err.c_str());
      return err;
    } else if (!root.isDirectory()) {
      std::string err = "!!!not a directory";
      PRINTLN(err.c_str());
      root.close();
      return err;
    }
    root.rewindDirectory();
    File file = root.openNextFile("r");
    PRINTLN("ls  :");
    if (!file) {
      std::string err = "no file present";
      PRINTLN(err.c_str());
      root.close();
      return err;
    }
    std::string res;

    while (file) {
      PRINT("  - ");
      PRINT(file.name());
      PRINT(" -> ");
      PRINTLN(file.size());
      auto relName = std::string(file.name()).substr(spiffPrefix.length());
      res += relName + '\n';
      file.close();
      file = root.openNextFile();
    }
    root.close();
    return res;
  }

  std::string getFile(std::string _path) {
    auto path = spiffPrefix + _path;
    const char *pc = &path[0];
    if (!SPIFFS.exists(pc)) {
      return "No file found";
    }
    File file = SPIFFS.open(pc, "r");

    auto Str = file.readString();
    file.close();
    std::string res(Str.c_str(), Str.length());

    return res;
  }

  void reboot() {
    delay(1000);
    ESP.restart();
  }
  GAPI gapi;
  std::string spiffPrefix;
  bool sendMat = true;
};
