#pragma once
#include <string>
#include <unistd.h>

class AudioPlayer {
    std::string currentUrl;
    pid_t playerPid = -1;
public:
    void play(const std::string& url);
    void stop();
};
