#include "WeatherTls.h"
#include "WeatherRootCertificates.h"

void configureWeatherTls(WiFiClientSecure& client) {
    client.setCACert(WeatherRootCertificates);
}