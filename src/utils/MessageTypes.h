#pragma once

struct camera_message {
  char message[32];
  int camera_action;
  int fps;
};
