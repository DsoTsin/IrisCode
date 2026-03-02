#include "zip.hpp"
#include "os.hpp"
#include "unzipper.h"
#include "zipper.h"
#include <array>
#include <fstream>

namespace iris {
void extract_zip(string const& file, string const& dir) {
  path::make(dir);
  ziputils::unzipper zipFile;
  zipFile.open(file.c_str());
  auto filenames = zipFile.getFilenames();
  for (auto it = filenames.begin(); it != filenames.end(); it++) {
    string path = *it;
    string final_dir = path::join(dir, path::file_dir(path));
    if (!path::exists(final_dir)) {
      path::make(final_dir);
    }
    zipFile.openEntry((*it).c_str());
    ofstream out(path::join(dir, path), ios::binary);
    zipFile >> out;
    out.close();
  }
}
void pack_zip(string const& file, string const& folder, string_list const& except) {
  ziputils::zipper zipFile;
  zipFile.open(file.c_str(), true);

  // add file into a folder
  /*ifstream file("unzipper.cpp", ios::in | ios::binary);
  if (file.is_open())
  {
      zipFile.addEntry("/Folder/unzipper.cpp");
      zipFile << file;
  }*/
  zipFile.close();
}
string zip_string(const string& str) {
  uLong cbound = ::compressBound(str.length() + 1);
  // uLongf length = 0;
  string ret;
  string temp;
  temp.reserve(cbound);
  if (Z_OK == ::compress((Bytef*)temp.data(), &cbound, (Bytef*)str.data(), str.length() + 1)) {
    ret.resize(cbound + 4 /*bytes*/);
    *(uint32_t*)ret.data() = str.length(); // compressed length
    memcpy(ret.data() + 4, temp.data(), cbound);
  }
  return ret;
}
string unzip_string(const string& str) {
  if (str.length() <= 4)
    return "";
  uint32_t olen = *(uint32_t*)str.data();
  string ret;
  ret.reserve(olen + 1);
  ret.data()[olen] = 0;
  ret.resize(olen);
  ::uncompress((Bytef*)ret.data(), (uLongf*)&olen, (const uint8_t*)str.data() + 4,
               str.length() - 4);
  return ret;
}

bool is_file_zip(const string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    return false; // Couldn't open file
  }
  std::array<char, 4> header;
  file.read(header.data(), header.size());
  if (file.gcount() != header.size()) {
    return false; // File too small
  }
  return (header[0] == 0x50 && header[1] == 0x4B &&
          header[2] == 0x03 && header[3] == 0x04);
}

bool is_file_7z(const string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    return false; // Couldn't open file
  }
  std::array<char, 6> header;
  file.read(header.data(), header.size());
  if (file.gcount() != header.size()) {
    return false; // File too small
  }
  return (header[0] == '7' && header[1] == 'z' && header[2] == 0xbc && header[3] == 0xaf
          && header[4] == 0x27 && header[5] == 0x1c);
}

} // namespace iris