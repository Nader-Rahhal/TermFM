#pragma once
#include <string>
#include <vector>
#include <curl/curl.h>
#include "Station.hpp"

class RadioBrowser {
    std::string host  = "de1.api.radio-browser.info";
    std::string codec = "MP3";
    std::vector<Station> stations;
    CURL* curl;

    std::string trim(const std::string& s);
    std::string urlEncode(const std::string& s);
    std::string get(const std::string& path);

public:
    RadioBrowser();
    ~RadioBrowser();
    std::vector<Station> getStations();
    std::vector<Station> searchByName(const std::string& name);
    std::vector<Station> searchByCountry(const std::string& country);
    std::vector<Station> searchByLanguage(const std::string& language);
};