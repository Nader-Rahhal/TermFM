#include "RadioBrowser.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <algorithm>
#include <simdjson.h>

std::string RadioBrowser::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string RadioBrowser::urlEncode(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    char* encoded = curl_easy_escape(curl, lower.c_str(), lower.size());
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

RadioBrowser::RadioBrowser() {
    curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) {
        static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
        return size * nmemb;
    });
}

RadioBrowser::~RadioBrowser() {
    curl_easy_cleanup(curl);
}

std::string RadioBrowser::get(const std::string& path) {
    std::string response;
    std::string url = "https://" + host + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(res));
    return response;
}

std::vector<Station> RadioBrowser::getStations() { return stations; }

static std::vector<Station> parseStations(std::string& body) {
    simdjson::ondemand::parser parser;
    // simdjson requires SIMDJSON_PADDING bytes of padding at the end
    simdjson::padded_string padded(body);
    auto doc = parser.iterate(padded);

    std::vector<Station> result;
    for (auto element : doc.get_array()) {
        std::string_view name, url;
        // get_string() returns string_view into the padded buffer
        if (element["name"].get_string().get(name) != simdjson::SUCCESS) name = "?";
        if (element["url"].get_string().get(url)   != simdjson::SUCCESS) url  = "?";
        result.push_back({std::string(name), std::string(url)});
    }
    return result;
}

std::vector<Station> RadioBrowser::searchByName(const std::string& name) {
    std::string body = get("/json/stations/byname/" + urlEncode(name) + "?codec=" + codec);
    return parseStations(body);
}

std::vector<Station> RadioBrowser::searchByCountry(const std::string& country) {
    std::string body = get("/json/stations/bycountry/" + urlEncode(country) + "?codec=" + codec);
    return parseStations(body);
}

std::vector<Station> RadioBrowser::searchByLanguage(const std::string& language) {
    std::string body = get("/json/stations/bylanguage/" + urlEncode(language) + "?codec=" + codec);
    return parseStations(body);
}
