#ifndef MADNESS_PARSEC_INCLUED
#define MADNESS_PARSEC_INCLUED

#include <madness/madness_config.h>
#warning "I'm here! step 1"

#ifdef HAVE_PARSEC
#warning "I'm here! step 2"
#include "madness/config.h"
#include <parsec.h>
#include <parsec/parsec_config.h>
#include <parsec/parsec_internal.h>
#include <parsec/devices/device.h>
#include <parsec/execution_stream.h>
#include <parsec/scheduling.h>  
#include <iostream>

namespace madness{
  extern "C"{

    extern const parsec_task_class_t madness_function;
    extern parsec_taskpool_t madness_handle;
  }

}

#endif // HAVE_PARSEC

#endif // MADNESS_PARSEC_INCLUED
