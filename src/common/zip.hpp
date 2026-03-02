#pragma once

#include "stl.hpp"

namespace iris
{
    extern void extract_zip(string const& file, string const& dir);
    extern void pack_zip(string const& file, string const& folder, string_list const& except);
    extern string zip_string(const string& str);
    extern string unzip_string(const string& str);
    extern bool is_file_zip(const string& path);
    extern bool is_file_7z(const string& path);
}