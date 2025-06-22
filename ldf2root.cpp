/**
  *@file ldf2root.cpp
  *@brief LDF to ROOT file converter for DDAS data
  *@author James Huffman
  *@date June 13, 2025
  *@param input_files Path to the LDF file(s) to be converted
  *@param config_file Path to the crate configuration file
  *@param output_file Path to the output ROOT file (optional, defaults to same name as input file with .root extension)
  *@param tree_name Name of the ROOT tree to create (default: "ddas/rawevents")
  *@param silent Suppress output messages (optional)
  *@param legacy Use legacy ROOT file output structure (optional)
*/

// Include necessary system headers
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <chrono>

// Include necessary ROOT headers
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <RtypesCore.h>

// GenScan Classes

#include "DataParser.h"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// Include additional user headers
#include "InputParser.h"
#include "DDASRootHit.h"
#include "DDASRootEvent.h"

void EventBuild(std::vector<DDASRootHit>* hitList, DDASRootEvent& rawEvent, ldf2root::CmdOptions opts, TBranch* hitBranch, const std::string& logname);

void generate_default_config(const std::string& filename = "example_config.txt") {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Failed to create " << filename << std::endl;
        return;
    }
    ofs << "# Example configuration for Pixie Crates\n";
    ofs << "# Format: sourceID(0) slotID(starts at 2) MSPS(100/250/500) ADC_resolution(12/14/16 bits) Hardware_revision(Rev F is current)\n";
    ofs << "# Be sure to rename this file if you want to use it! It will be overwritten if you run this program with --generate-config again.\n";
    for (int slot = 2; slot <= 14; ++slot) {
        ofs << "0 "<< slot << " 250 16 f\n";
    }
    ofs.close();
    std::cout << "Default config file '" << filename << "' generated.\n";
    std::cout << "Be sure to rename this file if you want to use it! It will be overwritten if you run this program with --generate-config again."<<std::endl;
    exit(0);
}

void PrintUsageString(std::ostream& os = std::cout) {
  os << "Usage: ldf2root [options]\n";
  os << "Options:\n";
  os << "  --generate-config      Generate a default configuration file\n";
  os << "  --help, -h             Show this help message\n";
  os << "  --input-file <file>    Path to the input LDF file (Required)\n";
  os << "  --config-file <file>   Path to the configuration file (Required)\n";
  os << "  --output-file <file>   Path to the output ROOT file (Optional: default input_files.ldf -> input_files.root)\n";
  os << "  --tree-name <name>     Name of the ROOT tree to create (default: 'ddas')\n";
  os << "  --build-window <time>  Build window in nanoseconds (default: 3000)\n";
  os << "  --window-type <type>   Type of window to use (0: flat, 1: fixed, 2: rolling; default: 1)\n";
  os << "  --silent               Suppress output messages\n";
  os << "  --legacy               ROOT file output uses legacy DDASEvent/ddaschannel object structure\n";
}

void parse_args(int argc, char* argv[], ldf2root::CmdOptions& opts) {

  // Parse command-line arguments
  if (argc < 2) {
    PrintUsageString(std::cerr);
    exit(0);
  }
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (std::string(argv[i]) == "--generate-config") {
      generate_default_config();
      exit(0);
    } else if (arg == "--help" || arg == "-h") {
      PrintUsageString();
      exit(0);
    } else if ((arg == "--input"|| arg=="-i" )&& i + 1 < argc) {
      opts.input_files.push_back(argv[++i]);
    } else if ((arg == "--output"|| arg=="-o") && i + 1 < argc) {
      opts.output_file = argv[++i];
    } else if ((arg == "--config"|| arg=="-c") && i + 1 < argc) {
      opts.config_file = argv[++i];
    } else if (arg == "--tree-name" && i + 1 < argc) {
      opts.tree_name = argv[++i];
    } else if (arg == "--build-window" && i + 1 < argc) {
      opts.build_window = std::stod(argv[++i]);
    } else if (arg == "--window-type" && i + 1 < argc) {
      int tmp = std::stoi(argv[++i]);
      if (tmp < 0 || tmp > 2) {
        std::cerr << "Invalid window type. Must be 0 (flat), 1(fixed), or 2 (rolling)." << std::endl;
      }
      if (tmp == 1) {
        opts.build_window_type = ldf2root::WindowType::FIXED;
      } else if (tmp == 2) {
        opts.build_window_type = ldf2root::WindowType::ROLLING;
      }
    } else if (arg == "--log-file") {
      opts.log_file = true;
    } else if (arg == "--silent") {
      opts.silent = true;
    } else if (arg == "--legacy") {
      opts.legacy = true;
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown option: " << arg << std::endl<<std::endl;
      PrintUsageString(std::cerr);
      exit(1);
    }
  }
  
  // Check if input file is specified
  if (opts.input_files.empty()) {
    std::cerr << "No input file specified." << std::endl;
    PrintUsageString(std::cerr);
    exit(1);
  }
  // Check if config file is specified
  if (opts.config_file.empty()) {
    std::cerr << "No config file specified." << std::endl;
    PrintUsageString(std::cerr);
    exit(1);
  }
  // Check input file ends with .ldf
  if (opts.input_files.at(0).size() < 4 || opts.input_files.at(0).substr(opts.input_files.size() - 4) != ".ldf") {
    std::cerr << "Input file must be of type .ldf." << std::endl;
    exit(1);
  }
  // Set default output file if not set
  if (opts.output_file.empty()) {
    opts.output_file = opts.input_files.at(0).substr(0, opts.input_files.size() - 4) + ".root";
  }
  // Set default tree name if not set
  if (opts.legacy) {
    opts.tree_name = "dchan";
    // legacy format is TTree name "dchan" with a branch "ddasevent" containing DDASEvent objects (These are std::vector<ddaschannel*> objects)
  } else if (opts.tree_name.empty()) {
    opts.tree_name = "ddas";
    // default format is TTree name "ddas" with a branch "rawevents" containing std::vector<DDASRootHit> objects
  }
  // Set output_stem to output_file name without extension
  size_t lastdot = opts.output_file.find_last_of('.');
  if (lastdot != std::string::npos) {
    opts.outfile_stem = opts.output_file.substr(0, lastdot);
  } else {
    opts.outfile_stem = opts.output_file;
  }
  opts.build_window *= 1e3; // Convert build window from microseconds to nanoseconds
}

bool ReadConfigFile(ldf2root::CmdOptions opts) {
  std::ifstream infile(opts.config_file);
  if (!infile.is_open()) {
    std::cerr << "Failed to open config file: " << opts.config_file << std::endl;
    return false;
  }
  std::string line;
  while (std::getline(infile, line)) {
    try{
      std::istringstream iss(line);
      if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments

      std::pair<unsigned int, unsigned int> crate_mod;
      unsigned int msps, res;
      std::string hw;

      if (!(iss >> crate_mod.first >> crate_mod.second >> msps >> res >> hw)) continue; // skip malformed lines
      opts.mod_params_map[crate_mod] = {msps, res, static_cast<unsigned int>(std::stol(hw,nullptr,16))}; // Store MSPS and resolution, third element is reserved for future use);
    } catch (const std::exception& e) {
      std::cerr << "Error parsing line in config file: " << line << "\n" << e.what() << std::endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char* argv[]) {
  std::chrono::time_point<std::chrono::high_resolution_clock> global_start_time = std::chrono::high_resolution_clock::now();
	const std::string logname = "ldf2root";
  // Parse command line arguments
  ldf2root::CmdOptions opts;
  parse_args(argc, argv, opts);
  std::string outfile_stem = opts.outfile_stem;

  // Read config file to get module MSPS mapping
  if( not ReadConfigFile(opts)){
    std::cerr << "Error reading config file: " << opts.config_file << std::endl;
    return 1;
  }

  if (opts.silent) {
    std::cout.setstate(std::ios_base::failbit); // Suppress output
  }

  std::cout << "Input file: " << opts.input_files.at(0) << std::endl;
  std::cout << "Output file: " << opts.output_file << std::endl;
  std::cout << "Config file: " << opts.config_file << std::endl;
  std::cout << "Tree name: " << opts.tree_name << std::endl;



  const std::string logfilename = (outfile_stem)+".log";
	const std::string errfilename = (outfile_stem)+".err";
	const std::string dbgfilename = (outfile_stem)+".dbg";

	spdlog::set_level(spdlog::level::debug);
	std::shared_ptr<spdlog::sinks::basic_file_sink_mt> LogFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfilename,true);
	LogFileSink->set_level(spdlog::level::info);

	std::shared_ptr<spdlog::sinks::basic_file_sink_mt> ErrorFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(errfilename,true);
	ErrorFileSink->set_level(spdlog::level::err);

	std::shared_ptr<spdlog::sinks::basic_file_sink_mt> DebugFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(dbgfilename,true);
	DebugFileSink->set_level(spdlog::level::debug);

	std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> LogFileConsole = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	LogFileConsole->set_level(spdlog::level::info);

	std::vector<spdlog::sink_ptr> sinks {DebugFileSink,LogFileSink,ErrorFileSink,LogFileConsole};
	auto console = std::make_shared<spdlog::logger>(logname,sinks.begin(),sinks.end());
	spdlog::initialize_logger(console);
	console->flush_on(spdlog::level::info);

  std::unique_ptr<DataParser> dataparser;
  dataparser.reset(new DataParser(DataParser::DataFileType::LDF_PIXIE, logname, opts));

    // Create output ROOT file and tree
  TFile* fout = TFile::Open(opts.output_file.c_str(), "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "Failed to create output ROOT file: " << opts.output_file << std::endl;
    return 1;
  }
  TTree* tout = new TTree(opts.tree_name.c_str(), "DDAS Unpacked Data");

  // Prepare DDASHit vector and branch
  DDASRootEvent dEvent;
  TBranch* hitBranch = nullptr;
  if (opts.legacy) {
    // Legacy format: TTree name "dchan" with a branch "ddasevent
    hitBranch = tout->Branch("dchan", &dEvent);
  } else {
    // Modern format: TTree name "ddas" with a branch "rawevents
    hitBranch = tout->Branch("rawevents", &dEvent);
  }

  // Main processing step
  auto rawHits = std::make_unique<std::vector<DDASRootHit>>();
  try {
    // Step 1: specify the input files to the DataParser
    dataparser->SetInputFiles(opts.input_files);
    // Step 2: parse the entire LDF file into DDASRootHit objects + store them in rawHits in time order
    dataparser->Parse(rawHits.get());
    fout->cd();
    // Step 3: Repack the DDASRootHit objects into DDASRootEvent objects and write them to the output ROOT file.
    EventBuild(rawHits.get(), dEvent, opts, hitBranch,logname);
    tout->Write("",TObject::kOverwrite);
    fout->Close();
  } catch(std::runtime_error const& e) {
    console->error(e.what());
    return 1;
  }

	auto global_run_time = std::chrono::high_resolution_clock::now() - global_start_time;
	const auto hrs = std::chrono::duration_cast<std::chrono::hours>(global_run_time);
	const auto mins = std::chrono::duration_cast<std::chrono::minutes>(global_run_time - hrs);
	const auto secs = std::chrono::duration_cast<std::chrono::seconds>(global_run_time - hrs - mins);
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(global_run_time - hrs - mins - secs);
	console->info("Finished converting in {} hours {} minutes {} seconds {} milliseconds",hrs.count(),mins.count(),secs.count(),ms.count());
	console->critical("All data has been written to {}.root",outfile_stem);

  return 0;
}

void EventBuild(std::vector<DDASRootHit>* hitList, DDASRootEvent& dEvent, ldf2root::CmdOptions opts, TBranch* hitBranch, const std::string& logname) {
  // Create a DDASRootEvent object to hold the unpacked data
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
  auto console = spdlog::get(logname)->clone("EventBuilder");
  Double_t lastTime = 0.0;
  dEvent.Clear();
  // Loop through each hit in the hitList
  switch(opts.build_window_type) {
    case (ldf2root::WindowType::FLAT):
      console->info("Building events with flat window type.");
      // For flat window, we can just fill the TTree with all hits
      for (auto& hit : *hitList) {
        dEvent.AddChannelData(&hit);
      }
      hitBranch->Fill();
      break;
    
    case (ldf2root::WindowType::ROLLING):
      // For rolling window, we need to keep track of the last event time
      lastTime = 0.0;
      console->info("Building events with rolling window type and build window of {} nanoseconds.", opts.build_window);
      for (auto& hit : *hitList) {
        if (std::abs(hit.getTime() - lastTime) < opts.build_window) {
          dEvent.AddChannelData(&hit);
          lastTime = hit.getTime();
        } else {
          // Fill the TTree with the current event
          hitBranch->Fill();
          dEvent.Clear();
          dEvent.AddChannelData(&hit);
          lastTime = hit.getTime();
        }
      }
      break;
    
    case (ldf2root::WindowType::FIXED):
      // For fixed window, we need to keep track of the last event time
      console->info("Building events with fixed window type and build window of {} nanoseconds.", opts.build_window);
      lastTime = 0.0;
      for ( auto& hit : *hitList) {
        if (std::abs(hit.getTime() - lastTime) < opts.build_window) {
          dEvent.AddChannelData(&hit);
        } else {
          // Fill the TTree with the current event
          hitBranch->Fill();
          dEvent.Clear();
          dEvent.AddChannelData(&hit);
          lastTime = hit.getTime();
        }
      }
      break;

    default:
      std::cerr << "Unknown window type specified." << std::endl;
      return;
  }
  std::chrono::time_point<std::chrono::high_resolution_clock> end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end_time - start_time;
  console->critical("Event build completed in {} seconds.", elapsed_seconds.count());
}