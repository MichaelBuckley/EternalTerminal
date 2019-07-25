#ifndef __ET_HEADERS__
#define __ET_HEADERS__

#if __FreeBSD__
#define _WITH_GETLINE
#endif

#if __APPLE__
#include <sys/ucred.h>
#include <util.h>
#elif __FreeBSD__
#include <libutil.h>
#include <sys/socket.h>
#elif __NetBSD__  // do not need pty.h on NetBSD
#include <util.h>
#else
#include <pty.h>
#include <signal.h>
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <ctime>
#include <deque>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <google/protobuf/message_lite.h>
#include "ET.pb.h"
#include "easylogging++.h"

#include <cxxopts.hpp>
#include "base64.hpp"
#include "json.hpp"
#include "sole.hpp"

#include "ThreadPool.h"

using namespace std;

namespace google {}
namespace gflags {}
using namespace google;
using namespace gflags;

using json = nlohmann::json;

// The ET protocol version supported by this binary
static const int PROTOCOL_VERSION = 6;

// Nonces for CryptoHandler
static const unsigned char CLIENT_SERVER_NONCE_MSB = 0;
static const unsigned char SERVER_CLIENT_NONCE_MSB = 1;

// system ssh config files
const string SYSTEM_SSH_CONFIG_PATH = "/etc/ssh/ssh_config";
const string USER_SSH_CONFIG_PATH = "/.ssh/config";

// Keepalive configs
const int CLIENT_KEEP_ALIVE_DURATION = 5;
// This should be at least double the value of CLIENT_KEEP_ALIVE_DURATION to
// allow enough time.
const int SERVER_KEEP_ALIVE_DURATION = 11;

#define FATAL_FAIL(X) \
  if (((X) == -1))    \
    LOG(FATAL) << "Error: (" << errno << "): " << strerror(errno);

// On BSD/OSX we can get EINVAL if the remote side has closed the connection
// before we have initialized it.
#define FATAL_FAIL_UNLESS_EINVAL(X)   \
  if (((X) == -1) && errno != EINVAL) \
    LOG(FATAL) << "Error: (" << errno << "): " << strerror(errno);

#ifndef ET_VERSION
#define ET_VERSION "unknown"
#endif

namespace et {
template <typename Out>
inline void split(const std::string& s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

inline std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

inline std::string SystemToStr(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result;
}

inline bool replace(std::string& str, const std::string& from,
                    const std::string& to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

inline int replaceAll(std::string& str, const std::string& from,
                      const std::string& to) {
  if (from.empty()) return 0;
  int retval = 0;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    retval++;
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();  // In case 'to' contains 'from', like replacing
                               // 'x' with 'yx'
  }
  return retval;
}

template <typename T>
inline T stringToProto(const string& s) {
  T t;
  if (!t.ParseFromString(s)) {
    LOG(FATAL) << "Error parsing string to proto: " << s.length() << " " << s;
  }
  return t;
}

template <typename T>
inline string protoToString(const T& t) {
  string s;
  if (!t.SerializeToString(&s)) {
    LOG(FATAL) << "Error serializing proto to string";
  }
  return s;
}

inline bool waitOnSocketData(int fd) {
  fd_set fdset;
  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  VLOG(4) << "Before selecting sockFd";
  FATAL_FAIL(select(fd + 1, &fdset, NULL, NULL, &tv));
  return FD_ISSET(fd, &fdset);
}

}  // namespace et

inline bool operator==(const google::protobuf::MessageLite& msg_a,
                       const google::protobuf::MessageLite& msg_b) {
  return (msg_a.GetTypeName() == msg_b.GetTypeName()) &&
         (msg_a.SerializeAsString() == msg_b.SerializeAsString());
}

inline bool operator!=(const google::protobuf::MessageLite& msg_a,
                       const google::protobuf::MessageLite& msg_b) {
  return (msg_a.GetTypeName() != msg_b.GetTypeName()) ||
         (msg_a.SerializeAsString() != msg_b.SerializeAsString());
}

#endif
