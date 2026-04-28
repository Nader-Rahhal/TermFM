#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include "json.hpp"
#include "Station.hpp"
using json = nlohmann::json;

class RadioBrowser {
private:
    std::string host  = "de1.api.radio-browser.info";
    std::string codec = "MP3";
    std::vector<Station> stations;

    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end   = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

    std::string get(const std::string& path) {
        addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0)
            throw std::runtime_error("getaddrinfo failed");
        int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (fd < 0) throw std::runtime_error("socket() failed");
        if (connect(fd, res->ai_addr, res->ai_addrlen) < 0)
            throw std::runtime_error("connect() failed");
        freeaddrinfo(res);
        std::string req = "GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
        send(fd, req.c_str(), req.size(), 0);
        std::string response;
        char buf[4096];
        ssize_t n;
        while ((n = recv(fd, buf, sizeof(buf), 0)) > 0)
            response.append(buf, n);
        close(fd);
        auto pos = response.find("\r\n\r\n");
        if (pos == std::string::npos)
            throw std::runtime_error("bad HTTP response");
        return response.substr(pos + 4);
    }

    void populateStations() {
        std::string body = get("/json/stations");
        std::vector<Station> newStations;
        for (auto& s : json::parse(body))
            if (s.value("codec", "") == codec)
                newStations.push_back({trim(s.value("name", "?")), s.value("url", "?")});
        stations = newStations;
    }

public:
    RadioBrowser() { populateStations(); }

    std::vector<Station> getStations() { return stations; }

    std::vector<Station> searchByName(const std::string& name) {
        std::string body = get("/json/stations/byname/" + name);
        std::vector<Station> result;
        for (auto& s : json::parse(body))
            if (s.value("codec", "") == codec)
                result.push_back({trim(s.value("name", "?")), s.value("url", "?")});
        return result;
    }

    std::vector<Station> searchByCountry(const std::string& country) {
        std::string body = get("/json/stations/bycountry/" + country);
        std::vector<Station> result;
        for (auto& s : json::parse(body))
            if (s.value("codec", "") == codec)
                result.push_back({trim(s.value("name", "?")), s.value("url", "?")});
        return result;
    }
};