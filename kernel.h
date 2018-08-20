#pragma once

#include "stats.h"
#include "clwrap.h"
#include "timeutil.h"
#include "common.h"

#include <string>
#include <vector>
#include <memory>

class Kernel {
  Holder<cl_kernel> kernel;
  cl_queue queue;
  int workGroups;
  std::string name;
  u64 timeSum;
  u64 nCalls;
  bool doTime;
  int groupSize;
  Stats stats;

  template<typename T> void setArgs(int pos, const T &arg) { ::setArg(kernel.get(), pos, arg); }
  
  template<typename T, typename... Args> void setArgs(int pos, const T &arg, const Args &...tail) {
    setArgs(pos, arg);
    setArgs(pos + 1, tail...);
  }

public:
  Kernel(cl_program program, cl_queue q, cl_device_id device, int workGroups, const std::string &name, bool doTime) :
    kernel(makeKernel(program, name.c_str())),
    queue(q),
    workGroups(workGroups),
    name(name),
    doTime(doTime),
    groupSize(getWorkGroupSize(kernel.get(), device, name.c_str()))
  {
    // assert((workSize % groupSize == 0) || (log("%s\n", name.c_str()), false));
  }

  template<typename... Args> void setFixedArgs(int pos, Args &...tail) { setArgs(pos, tail...); }
  
  template<typename... Args> void operator()(const Args &...args) {
    setArgs(0, args...);
    run(workGroups);
  }

  void run(u32 nWorkGroups) {
    if (doTime) {
      finish(queue);
      Timer timer;
      ::run(queue, kernel.get(), groupSize, nWorkGroups * groupSize, name);
      finish(queue);
      stats.add(timer.deltaMicros());
    } else {
      ::run(queue, kernel.get(), groupSize, nWorkGroups * groupSize, name);
    }
  }
  
  string getName() { return name; }

  StatsInfo resetStats() {
    StatsInfo ret = stats.getStats();
    stats.reset();
    return ret;
  }
};
