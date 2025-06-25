#ifndef __DATA_PARSER_HPP__
#define __DATA_PARSER_HPP__

#include <memory>
#include <string>
#include <vector>

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Translator.h"
#include "InputParser.h"


class DataParser{
	public:
		enum DataFileType{
			Unknown,
			CAEN_ROOT,
			CAEN_BIN,
			LDF_PIXIE,
			PACMAN_LDF_PIXIE,
			PLD,
			EVT_PRESORT,
			EVT_BUILT
		};
		DataParser(DataFileType,const std::string&, ldf2root::CmdOptions cmdopts);
		~DataParser() = default;
		void SetInputFiles(std::vector<std::string>&);
		
		Translator::TRANSLATORSTATE Parse(std::vector<uint32_t>* RawEvents);

	private:
		DataFileType DataType;
		std::shared_ptr<spdlog::logger> console;
		std::string LogName;
		std::string ParserName;
		ldf2root::CmdOptions CmdOpts;

		std::unique_ptr<Translator> DataTranslator;
};

#endif
