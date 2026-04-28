#include "RadioBrowser.hpp"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;

std::string RadioBrowser::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string RadioBrowser::urlEncode(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string out;
    for (unsigned char c : lower) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

std::string RadioBrowser::get(const std::string& path) {
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

void RadioBrowser::populateStations() {
    std::string body = get("/json/stations");
    std::vector<Station> newStations;
    for (auto& s : json::parse(body))
        if (s.value("codec", "") == codec)
            newStations.push_back({trim(s.value("name", "?")), s.value("url", "?")});
    stations = newStations;
}

RadioBrowser::RadioBrowser() { populateStations(); }

std::vector<Station> RadioBrowser::getStations() { return stations; }

std::vector<Station> RadioBrowser::searchByName(const std::string& name) {
    std::string body = get("/json/stations/byname/" + urlEncode(name));
    std::vector<Station> result;
    for (auto& s : json::parse(body))
        if (s.value("codec", "") == codec)
            result.push_back({trim(s.value("name", "?")), s.value("url", "?")});
    return result;
}

std::vector<Station> RadioBrowser::searchByCountry(const std::string& country) {
    std::string body = get("/json/stations/bycountry/" + urlEncode(country));
    std::vector<Station> result;
    for (auto& s : json::parse(body))
        if (s.value("codec", "") == codec)
            result.push_back({trim(s.value("name", "?")), s.value("url", "?")});
    return result;
}

std::vector<Station> RadioBrowser::searchByLanguage(const std::string& language) {
    std::string body = get("/json/stations/bylanguage/" + urlEncode(language));
    std::vector<Station> result;
    for (auto& s : json::parse(body))
        if (s.value("codec", "") == codec)
            result.push_back({trim(s.value("name", "?")), s.value("url", "?")});
    return result;
}
