#include <gst/gst.h>
#include <glib.h>
#include <json-glib-1.0/json-glib/json-glib.h>
#include <iostream>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>
#include <functional>
#include <map>
#include <set>
#include <cstring>      // Для memset()
#include <cerrno>       // Для errno

#include <fcntl.h>      // Для open()
#include <unistd.h>     // Для close()
#include <sys/ioctl.h>  // Для ioctl()
#include <linux/videodev2.h> // Структуры V4L2

#include "utils.h"
