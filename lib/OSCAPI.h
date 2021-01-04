#pragma once
#include "API/API.h"
#include "OSCMessage.h"
#include <map>
#include <string>
using std::string;

class OSCAPI : public API<OSCAPI> {
public:
  std::unique_ptr<APIInstance<OSCAPI>> selfAPI;
  OSCAPI() {

    // rFunction<std::string>("listNamespaces", [](OSCAPI &a) -> std::string {
    //   std::vector<std::string> nsNames;
    //   for (auto const &imap : a.apis)
    //     nsNames.push_back(imap.first + ":" + imap.second->getAPI().toString()
    //     +
    //                       "\n");
    //   return StringHelpers::joinIntoString(nsNames);
    // });

    selfAPI.reset(new APIInstance<OSCAPI>(*this, *this));
    registerAPI("/o", selfAPI.get());
  };

  void registerAPI(const string &ns, APIInstanceBase *api) {
    apis.emplace(ns, api);
  }

  typedef enum { NONE = 0, SET = 1, GET = 2, CALL = 3 } MsgType;

  MsgType getType(OSCMessage &msg) {
    if (msg.getString(0, OSCEndpoint::getBuf()) > 0) {
      if (strcmp(OSCEndpoint::getBuf(), "get") == 0) {
        return GET;
      } else if (strcmp(OSCEndpoint::getBuf(), "call") == 0) {
        return CALL;
      } else if (strcmp(OSCEndpoint::getBuf(), "set") == 0) {
        return SET;
      }
    }
    return NONE;
  }

  TypedArgList listFromOSCMessage(OSCMessage &msg, int start = 0) {
    TypedArgList res;
    for (int i = start; i < msg.size(); i++) {
      res.args.push_back(argFromOSCMessage(msg, i));
    }
    return res;
  }

  TypedArgBase::UPtr argFromOSCMessage(OSCMessage &msg, int o) {
    if (msg.isFloat(o)) {
      return std::make_unique<TypedArg<float>>(msg.getFloat(o));
    } else if (msg.isInt(o)) {
      return std::make_unique<TypedArg<int>>(msg.getInt(o));
    } else if (msg.isDouble(o)) {
      return std::make_unique<TypedArg<double>>(msg.getDouble(o));
    } else if (msg.isBoolean(o)) {
      return std::make_unique<TypedArg<bool>>(msg.getBoolean(o));
    } else if (msg.isString(o)) {
      std::string s(msg.getDataLength(o), 0);
      msg.getString(o, &s[0]);
      return std::make_unique<TypedArg<std::string>>(s);
    }
    Serial.println("unsuported OSC member");
    return {};
  }

  bool listToOSCMessage(const TypedArgList &l, OSCMessage &msg, int start = 0);

  template <typename T>
  bool addToMsg(const TypedArgBase &a, OSCMessage &msg) const {
    if (a.is<T>()) {
      msg.add(a.get<T>());
      return true;
    }
    return false;
  }

  QResult processOSC(OSCMessage &msg, bool &needAnswer) {
    needAnswer = false;
    if (auto a = getEpFromMsg(msg)) {
      auto &api = *a.api;
      auto mt = getType(msg);
      Identifier localName = a.getLocalIdentifier(msg);
      if (localName.size() && localName[0] == '/') {
        localName = localName.substr(1);
      }
      Serial.print("msg type : ");
      Serial.print((int)mt);
      Serial.print("local name : ");
      Serial.println(localName.c_str());
      if (localName.size()) {
#if 1
        if (mt == GET) {
          needAnswer = true;
          return api.get(localName);
        }
        if (mt == SET) {
          bool success = api.set(localName, {argFromOSCMessage(msg, 1)});
          return QResult::ok();
        }
        if (mt == CALL) {
          needAnswer = true;
          return api.call(localName, listFromOSCMessage(msg, 1));
        } else {
          // defaults
          if (msg.size() == 1) { // set if argument
            if (api.canSet(localName)) {
              bool success = api.set(localName, {argFromOSCMessage(msg, 0)});
              return QResult(success);
            } else if (api.canCall(localName)) {
              needAnswer = true;
              return api.call(localName, {argFromOSCMessage(msg, 0)});
            } else {
              return QResult::err("1 arg but can't set nor call");
            }
          } else if (msg.size() == 0) {
            if (api.canGet(localName)) { //  get if can get none
              needAnswer = true;
              return api.get(localName);
            } else if (api.canCall(localName)) {
              needAnswer = true;
              return api.call(localName, TypedArgList::empty());
            } else {
              return QResult::err("no arg but can't get nor call");
            }
          } else { // multi args
            if (api.canCall(localName)) {
              needAnswer = true;
              return api.call(localName, listFromOSCMessage(msg));
            } else {
              return QResult::err("multi args but can't call");
            }
          }
        }
#endif
      }

      return QResult::err(" can't reslove local name");
    }
    return QResult::err(" can't reslove api for namespace");
  }

  struct OSCEndpoint {
    APIInstanceBase *api = nullptr;
    int matchOffset = -1;
    static char *getBuf() {
      static char strBuf[255];
      return strBuf;
    }

    MsgType type = SET;
    std::string getRemainingAddress(OSCMessage &msg) {
      if (matchOffset >= 0) {
        if (msg.getAddress(getBuf(), matchOffset)) {
          return std::string(getBuf());
        }
      }
      return {};
    }

    std::string getLocalIdentifier(OSCMessage &msg) {
      auto rem = getRemainingAddress(msg);
      if (rem.size()) {
        return rem;
      }
      if (msg.getString(0, getBuf())) {
        return std::string(getBuf());
      }
      return {};
    }

    operator bool() const { return api != nullptr; }
  };

protected:
  OSCEndpoint getEpFromMsg(const OSCMessage &msg) {
    for (auto &a : apis) {
      int partialMatchOffset =
          const_cast<OSCMessage *>(&msg)->match(a.first.c_str());
      Serial.print("matches : ");
      Serial.println(partialMatchOffset);
      if (partialMatchOffset > 0) {
        return {.api = (APIInstanceBase *)(a.second),
                .matchOffset = partialMatchOffset};
      }
    }
    return {};
  }
  std::map<string, APIInstanceBase *> apis;
};

template <>
bool OSCAPI::addToMsg<std::string>(const TypedArgBase &a,
                                   OSCMessage &msg) const {
  if (a.is<std::string>()) {
    auto s = a.get<std::string>();
    msg.add(&s[0]);
    return true;
  }
  return false;
}

bool OSCAPI::listToOSCMessage(const TypedArgList &l, OSCMessage &msg,
                              int start) {
  bool success = l.size() - start > 0;
  if (!success) {
    Serial.println("nothing to add in resp msg");
  }
  for (int i = start; i < l.size(); i++) {
    auto &a = *l[i];
    bool added = addToMsg<bool>(a, msg) || addToMsg<int>(a, msg) ||
                 addToMsg<float>(a, msg) || addToMsg<double>(a, msg) ||
                 addToMsg<std::string>(a, msg);
    if (!added) {
      Serial.println("can't add to resp msg");
    }
    success &= added;
  }
  return success;
}
