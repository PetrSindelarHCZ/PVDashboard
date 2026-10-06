#include "WeatherTls.h"
#include "WeatherCaBundle.h"

void configureWeatherTls(WiFiClientSecure& client) {
    client.setCACertBundle(WeatherCaBundle);
}
