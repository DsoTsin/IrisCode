#include <filesystem>
#include <fstream>
#include <iostream>
#include <lzfse.h>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, const char* argv[]) {
  std::vector<fs::path> paths;
  if (argc >= 2) {
    for (int i = 1; i < argc; i++) {
      paths.push_back(argv[i]);
    }
  } else {
    paths.push_back(fs::current_path());
  }

  size_t index = 0;
  for (auto& path : paths)
    for (auto iterEntry = fs::recursive_directory_iterator(path);
         iterEntry != fs::recursive_directory_iterator(); ++iterEntry) {
      const auto filenameStr = iterEntry->path().filename().string();
      if (iterEntry->is_regular_file()) {
        auto path = iterEntry->path().string();
        std::ifstream file(iterEntry->path(), std::ios::binary | std::ios::in);

        file.seekg(0, std::ios_base::end);
        size_t length = file.tellg();
        file.seekg(0, std::ios_base::beg);

        std::vector<uint8_t> buffer;
        buffer.reserve(length);
        std::copy(std::istreambuf_iterator<char>(file),
                  std::istreambuf_iterator<char>(),
                  std::back_inserter(buffer));

        std::vector<uint8_t> obuffer;
        obuffer.resize(length);
        size_t compressed = lzfse_encode_buffer(obuffer.data(), obuffer.size(), buffer.data(), buffer.size(), nullptr);

        std::cout << index << " Compressed: " << compressed << " Origin: " << length
                  << " file:  " << path << '\n';
        index++;
      }
    }
  return 0;
}