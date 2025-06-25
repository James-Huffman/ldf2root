#ifndef __TRANSLATOR_HPP__
#define __TRANSLATOR_HPP__

#include <memory>
#include <string>
#include <fstream>
#include <vector>

#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


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
		[[noreturn]] virtual TRANSLATORSTATE Parse([[maybe_unused]] std::vector<uint32_t>*);
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

		std::shared_ptr<spdlog::logger> console;

		uint64_t CurrExtTS;
};
/// @}

#endif
