// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "debugger.h"


#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

#if defined(__GLIBCXX__)
#include <cxxabi.h>
#endif

#include <iostream>

#include "basictypes.h"

#include "logging.h"
#include "scoped_ptr.h"
#include "safe_strerror_posix.h"
#include "string_piece.h"


#if defined(USE_SYMBOLIZE)
#include "symbolize.h"
#endif


namespace debug {

bool SpawnDebuggerOnProcess(unsigned /* process_id */) {
  //NOTIMPLEMENTED();
  return false;
}



// We can look in /proc/self/status for TracerPid.  We are likely used in crash
// handling, so we are careful not to use the heap or have side effects.
// Another option that is common is to try to ptrace yourself, but then we
// can't detach without forking(), and that's not so great.
// static
bool BeingDebugged() {
  int status_fd = open("/proc/self/status", O_RDONLY);
  if (status_fd == -1)
    return false;

  // We assume our line will be in the first 1024 characters and that we can
  // read this much all at once.  In practice this will generally be true.
  // This simplifies and speeds up things considerably.
  char buf[1024];

  ssize_t num_read = HANDLE_EINTR(read(status_fd, buf, sizeof(buf)));
  if (HANDLE_EINTR(close(status_fd)) < 0)
    return false;

  if (num_read <= 0)
    return false;

  StringPiece status(buf, num_read);
  StringPiece tracer("TracerPid:\t");

  StringPiece::size_type pid_index = status.find(tracer);
  if (pid_index == StringPiece::npos)
    return false;

  // Our pid is 0 without a debugger, assume this for any pid starting with 0.
  pid_index += tracer.size();
  return pid_index < status.size() && status[pid_index] != '0';
}


// We want to break into the debugger in Debug mode, and cause a crash dump in
// Release mode. Breakpad behaves as follows:
//
// +-------+-----------------+-----------------+
// | OS    | Dump on SIGTRAP | Dump on SIGABRT |
// +-------+-----------------+-----------------+
// | Linux |       N         |        Y        |
// | Mac   |       Y         |        N        |
// +-------+-----------------+-----------------+
//
// Thus we do the following:
// Linux: Debug mode, send SIGTRAP; Release mode, send SIGABRT.
// Mac: Always send SIGTRAP.

#if defined(NDEBUG) && !defined(OS_MACOSX)
#define DEBUG_BREAK() abort()
#elif defined(OS_NACL)
// The NaCl verifier doesn't let use use int3.  For now, we call abort().  We
// should ask for advice from some NaCl experts about the optimum thing here.
// http://code.google.com/p/nativeclient/issues/detail?id=645
#define DEBUG_BREAK() abort()
#elif defined(ARCH_CPU_ARM_FAMILY)
#define DEBUG_BREAK() asm("bkpt 0")
#else
#define DEBUG_BREAK() asm("int3")
#endif

void BreakDebugger() {
  DEBUG_BREAK();
#if defined(NDEBUG)
  _exit(1);
#endif
}

}  // namespace debug
