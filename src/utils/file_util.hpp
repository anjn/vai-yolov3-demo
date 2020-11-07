#pragma once
#include <iostream>
#include <fstream>

namespace demo {
  
static void assert_file_exists(const std::string& file)
{
  std::ifstream ifs(file);
  if (!ifs.is_open()) {
    std::cerr << "Error:  '" << file << "' doesn't exist!" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << file << std::endl;
}

}
