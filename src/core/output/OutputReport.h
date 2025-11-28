#pragma once

#include <iostream>

#include "analysis/CycleDetector.h"

class OutputReport {
public:

    static void GenerateReport(const std::vector<Cycle>& cycles, const std::string& format);
};