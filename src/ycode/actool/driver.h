#pragma once

#include "common/stl.hpp"

namespace iris {
class ActoolDriver {
public:

  struct Options {
    int outputFormat;
    int logFlags; // notices, warnings,
    string outputInfoPlist;
    string appIcon;
    string launchImage;
    string productType;
    string minimumDeploymentTarget;
    string_list platform;
    string_list targetDevice;
    string_list modelFilter;
    string_list osFilter;
    bool onDemandResource;
    string outputDir;
    string inputDir;
  };
};
} // namespace iris