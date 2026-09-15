#pragma once
#include <Arduino.h>
#include "ConfigSchema.h"

String exportConfigurationYaml(const AppConfig& config);
bool importConfigurationYaml(const String& yaml, AppConfig& config, String& error);
