#ifndef __TRANSLATOR_HPP__
#define __TRANSLATOR_HPP__

#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <deque>

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// #include <boost/container/devector.hpp>
// #include <boost/container/vector.hpp>
// #include <boost/container/deque.hpp>

// #include "BitDecoder.h"
// #include "ChannelMap.hpp"

#include "DDASRootHit.h"

/// @addtogroup Decoding
/// @{
/// @class Translator
class Translator{
	public:
		enum TRANSLATORSTATE{
			PARSING,
			COMPLETE,
			UNKNOWN
		};
		Translator(const std::string&,const std::string&);
		virtual ~Translator();
		virtual bool AddFile(const std::string&);
		[[noreturn]] virtual TRANSLATORSTATE Parse([[maybe_unused]] std::unique_ptr<std::vector<std::unique_ptr<DDASRootHit>>>&);
		virtual void FinalizeFiles();
		virtual bool OpenNextFile();

	protected:
		std::string LogName;
		std::string TranslatorName;

		std::vector<std::string> InputFiles;
		std::vector<int> FileSizes;
		std::ifstream CurrentFile;
		size_t NumTotalFiles;
		size_t NumFilesRemaining;
		size_t CurrentFileIndex;
		bool FinishedCurrentFile;
		bool FirstFile;
		bool LastFile;

		bool LastReadEvtWithin;
		std::vector<std::deque<std::unique_ptr<DDASRootHit>>> CustomLeftovers;
		std::vector<std::deque<uint64_t>> LeftoverSpillIDs;
		// boost::container::vector<boost::container::deque<PhysicsData>> CustomLeftovers;

		std::shared_ptr<spdlog::logger> console;
		// std::shared_ptr<ChannelMap> CMap;
		// std::shared_ptr<Correlator> correlator;

		// XiaDecoder* CurrDecoder;

		uint64_t CurrExtTS;
};
/// @}

#endif
