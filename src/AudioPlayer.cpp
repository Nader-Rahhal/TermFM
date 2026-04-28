#include "AudioPlayer.hpp"
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

void AudioPlayer::play(const std::string& url) {
    stop();
    currentUrl = url;
    playerPid = fork();
    if (playerPid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
        execlp("ffplay", "ffplay", "-nodisp", url.c_str(), nullptr);
        exit(1);
    }
}

void AudioPlayer::stop() {
    if (playerPid > 0) {
        kill(playerPid, SIGTERM);
        waitpid(playerPid, nullptr, 0);
        playerPid = -1;
    }
}
