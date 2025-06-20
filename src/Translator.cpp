#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

#include "Translator.h"

Translator::Translator(const std::string& log,const std::string& translatorname){
	this->LogName = log;
	this->TranslatorName = translatorname;

	this->console = spdlog::get(this->LogName)->clone(this->TranslatorName);
	this->console->info("Created Translator [{}]",this->TranslatorName);
	
	this->LastReadEvtWithin = false;
	this->CurrExtTS = std::numeric_limits<uint64_t>::max();

	this->CustomLeftovers = std::vector<std::deque<DDASRootHit>>(13,std::deque<DDASRootHit>());
}

Translator::~Translator(){
	if( not this->CurrentFile.eof() or this->CurrentFileIndex < this->NumTotalFiles ){
		this->console->error("Translator didn't finish reading final file");
	}
}
		
bool Translator::AddFile(const std::string& filename){
	std::filesystem::path filepath(filename);
	if( std::filesystem::exists(filepath) ){
		this->FileSizes.push_back(std::filesystem::file_size(filepath));
		this->InputFiles.push_back(filename);
		this->console->info("Added File {} to list of files to translate, File Size : {}",this->InputFiles.back(),this->FileSizes.back());
		return true;
	}else{
		return false;
	}
}

[[noreturn]] Translator::TRANSLATORSTATE Translator::Parse([[maybe_unused]] std::vector<DDASRootHit>* RawEvents){
	this->console->error("Called Translator::Parse(), not the overload");
	throw std::runtime_error("Called Translator::Parse(), not the overload");
}

void Translator::FinalizeFiles(){
	this->NumTotalFiles = this->InputFiles.size();
	this->NumFilesRemaining = this->NumTotalFiles;
	this->CurrentFileIndex = 0;
	this->FinishedCurrentFile = true;
}

bool Translator::OpenNextFile(){
	this->FinishedCurrentFile = false;
	if( this->CurrentFileIndex == 0 ){
		this->console->info("Opening First File : {}",this->InputFiles.at(this->CurrentFileIndex));
		this->CurrentFile.open(this->InputFiles.at(this->CurrentFileIndex),std::ifstream::binary);
		++(this->CurrentFileIndex);
		return true;
	}else if(this->CurrentFileIndex == this->NumTotalFiles){
		this->console->info("Completed Final File : {}",this->InputFiles.at(this->CurrentFileIndex-1));
		this->CurrentFile.close();
		return false;
	}else{
		this->console->info("Swapping input File from : {} to : {}",this->InputFiles.at(this->CurrentFileIndex-1),this->InputFiles.at(this->CurrentFileIndex));
		this->console->info("{}/{} Files Processed",this->CurrentFileIndex,this->InputFiles.size());
		this->CurrentFile.close();
		this->CurrentFile.open(this->InputFiles.at(this->CurrentFileIndex),std::ifstream::binary);
		++(this->CurrentFileIndex);
		return true;
	}
}

// void Translator::SetChannelMap(const std::shared_ptr<ChannelMap>& cmap){
// 	this->CMap = cmap;
// }

// void Translator::SetCorrelator(const std::shared_ptr<Correlator>& corr){
// 	this->correlator = corr;
// }
