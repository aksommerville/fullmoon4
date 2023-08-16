#ifndef MSAUDIO_INTERNAL_H
#define MSAUDIO_INTERNAL_H

#include "opt/bigpc/bigpc_audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

struct bigpc_audio_driver_msaudio {
  struct bigpc_audio_driver hdr;
  HWAVEOUT waveout;
  WAVEHDR bufv[2];
  int bufp;
  HANDLE thread;
  HANDLE thread_terminate;
  HANDLE thread_complete;
  HANDLE buffer_ready;
  HANDLE mutex;
};

#define DRIVER ((struct bigpc_audio_driver_msaudio*)driver)

#endif
