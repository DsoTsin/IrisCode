#include "asset_catalog.h"

#include "driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char* argv[]) {
  if (argc == 1) {
    //...
  } else if (argc == 2) {
    if (!strcmp(argv[1], "--version")) {
      printf("actool 0.1.0");
      return 0;
    } else if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {

    } else {
    }
  } else if (argc == 3) {
    if (!strcmp(argv[1], "--print-contents")) {
      const char* path = argv[2];
      iris::AssetCatalog catalog;
      catalog.load(path);
      return 0;
    } else { // ..
    }
  } else {
    int argid = 1;
    iris::ActoolDriver::Options options;
    while (argid < argc) {
      if (!strcmp(argv[argid], "--notices")) {
        options.logFlags;
      } else if (!strcmp(argv[argid], "--warnings")) {
        options.logFlags;
      } else if (!strcmp(argv[argid], "--output-format")) {
        argid++;
        options.outputFormat;
      } else if (!strcmp(argv[argid], "--output-partial-info-plist")) {
        argid++;
        options.outputInfoPlist = argv[argid];
      } else if (!strcmp(argv[argid], "--app-icon")) {
        argid++;
        options.appIcon = argv[argid];
      } else if (!strcmp(argv[argid], "--launch-image")) {
        argid++;
        options.launchImage = argv[argid];
      } else if (!strcmp(argv[argid], "--filter-for-device-model")) {
        argid++;
        options.modelFilter << argv[argid];
      } else if (!strcmp(argv[argid], "--filter-for-device-os-version")) {
        argid++;
        options.osFilter << argv[argid];
      } else if (!strcmp(argv[argid], "--product-type")) {
        argid++;
        options.productType = argv[argid];
      } else if (!strcmp(argv[argid], "--target-device")) {
        argid++;
        options.targetDevice << argv[argid];
      } else if (!strcmp(argv[argid], "--platform")) {
        argid++;
        options.platform << argv[argid];
      } else if (!strcmp(argv[argid], "--minimum-deployment-target")) {
        argid++;
        options.minimumDeploymentTarget = argv[argid];
      } else if (!strcmp(argv[argid], "--enable-on-demand-resources")) {
        argid++;
        options.onDemandResource = true;
      } else if (!strcmp(argv[argid], "--compile")) {
        argid++;
        options.outputDir = argv[argid];
        argid++;
        options.inputDir = argv[argid];
      } else { // unsupported argument
        printf("Unsupported argument %s.", argv[argid]);
        return 1;
      }
      argid++;
    } // arg parse finished
  }

  return 0;
}