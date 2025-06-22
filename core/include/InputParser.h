#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <array>
#include <RtypesCore.h>

namespace ldf2root {
enum WindowType {
  FLAT = 0,
  FIXED = 1,
  ROLLING = 2
};

struct CmdOptions {
  std::map<std::pair<unsigned int, unsigned int>, std::array<unsigned int,3>> mod_params_map;
  std::vector<std::string> input_files;
  std::string output_file;
  std::string outfile_stem;
  std::string config_file;
  std::string tree_name;
  Double_t build_window = 3000; // Default build window in nanoseconds
  WindowType build_window_type = WindowType::FLAT; // Default to fixed window
  Bool_t log_file = false;
  Bool_t silent = false;
  Bool_t legacy = false;
};
}

#endif