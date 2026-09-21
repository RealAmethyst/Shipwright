#pragma once

#include "soh/NativeOptions/NativeOptions.h"

std::vector<NativeOptions::Row> ActorParameterRows(uint16_t id, uint16_t params,
                                                  std::function<void(uint16_t)> set);
