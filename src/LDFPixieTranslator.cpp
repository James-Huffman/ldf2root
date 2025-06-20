#include <cstdint>
#include <cstdlib>
#include <ios>
#include <limits>
#include <stdexcept>
#include <algorithm>

// #include <boost/sort/pdqsort/pdqsort.hpp>
// #include <boost/sort/spinsort/spinsort.hpp>

#include "LDFPixieTranslator.h"
// #include "BitDecoder.hpp"
// #include "PhysicsData.hpp"
#include "DDASRootHit.h"
#include "Translator.h"
#include "DDASHit.h"
#include "DDASHitUnpacker.h"
// #include "boost/move/iterator.hpp"



LDFPixieTranslator::LDFPixieTranslator(const std::string& logname,const std::string& translatorname, const ldf2root::CmdOptions& cmdopts) : Translator(logname,translatorname){
	this->PrevTimeStamp = 0;
	this->CurrSpillID = 0;
	this->EvtSpillCounter = std::vector<int>(this->NUMCONCURRENTSPILLS,0);
	this->FinishedReadingFiles = false;
	this->CmdOpts = cmdopts;
	this->CurrDirBuff = { 
		.dirBuffType = HRIBF_TYPES::DIR, 
		.dirBufferSize = 8192, 
		.fileBufferSize = 8194,
		.totalFileBuffers = 0, 
		.unknown = { 0, 1, 2}, 
		.run_num = 0 
	};
	this->CurrHeadBuff = { 
		.bufftype = HRIBF_TYPES::HEAD, 
		.buffsize = 64, 
		.facility = {'N','U','L','L','\0'}, 
		.format = {'N','U','L','L','\0'}, 
		.type = {'N','U','L','L','\0'},
		.date = {'N','U','L','L','\0'},
		.run_title = {'N','U','L','L','\0'},
		.run_num = 0
	}; 
	this->CurrDataBuff = {
		.bufftype = HRIBF_TYPES::DATA,
		.buffsize = 8192,
		.bcount = 0,
		.buffhead = 0,
		.nextbuffhead = 0,
		.nextbuffsize = 0,
		.goodchunks = 0,
		.missingchunks = 0,
		.numbytes = 0,
		.numchunks = 0,
		.currchunknum = 0,
		.prevchunknum = 0,
		.buffpos = 0,
		.currbuffer = nullptr,
		.nextbuffer = nullptr,
		.buffer1 = std::vector<uint32_t>(this->CurrDirBuff.fileBufferSize,0xFFFFFFFF),
		.buffer2 = std::vector<uint32_t>(this->CurrDirBuff.fileBufferSize,0xFFFFFFFF)
	};
	this->NTotalWords = 0;
}

LDFPixieTranslator::~LDFPixieTranslator(){
	this->console->info("good chunks : {}, bad chunks : {}, spills : {}",this->CurrDataBuff.goodchunks,this->CurrDataBuff.missingchunks,this->CurrSpillID);
	int idx = 0;
	for( const auto& mod : this->CustomLeftovers ){
		if( not mod.empty() ){
			this->console->critical("Leftover Events Module {} : {}",idx,mod.size());
		}
		++idx;
	}
}

Translator::TRANSLATORSTATE LDFPixieTranslator::Parse(std::vector<DDASRootHit>* RawEvents){
	if( this->InputFiles.empty() ){
		this->console->error("No input files to parse");
		return Translator::TRANSLATORSTATE::COMPLETE;
  }
	if( this->FinishedCurrentFile ){
		if( this->OpenNextFile() ){
			if( this->ParseDirBuffer() == -1 ){
				throw std::runtime_error("Invalid Dir Buffer when opening file : "+this->InputFiles.at(this->CurrentFileIndex));
			}
			if( this->ParseHeadBuffer() == -1 ){
				throw std::runtime_error("Invalid Head Buffer when opening file : "+this->InputFiles.at(this->CurrentFileIndex));
			}
			this->CurrDataBuff.bcount = 0;
		}else{
			this->FinishedReadingFiles = true;
		}
	}
	std::vector<bool> entriesread(this->CustomLeftovers.size(),false);
	while( not this->FinishedReadingFiles and (this->CountBuffersWithData() < this->NUMCONCURRENTSPILLS) ){
		if( this->CurrentFile.eof() ){
			if( this->OpenNextFile() ){
				if( this->ParseDirBuffer() == -1 ){
					throw std::runtime_error("Invalid Dir Buffer when opening file : "+this->InputFiles.at(this->CurrentFileIndex));
				}
				if( this->ParseHeadBuffer() == -1 ){
					throw std::runtime_error("Invalid Head Buffer when opening file : "+this->InputFiles.at(this->CurrentFileIndex));
				}
				this->CurrDataBuff.bcount = 0;
			}else{
				this->FinishedReadingFiles = true;
			}
		}
		if( this->FinishedReadingFiles ){
			break;
		}

		bool full_spill;
		bool bad_spill;
		uint32_t nBytes = 0;
		int retval = this->ParseDataBuffer(nBytes,full_spill,bad_spill);
		if( retval == -1 ){
			throw std::runtime_error("Invalid Data Buffer in File : "+this->InputFiles.at(this->CurrentFileIndex));
		}
		// Read in complete file and had no spill errors
		if( full_spill and  retval != 2){
			this->UnpackData(nBytes,full_spill,bad_spill,entriesread, RawEvents);
		}
	}
	// Sort the data within each module 
	for( size_t ii = 0; ii < entriesread.size(); ++ii ){
		if( entriesread[ii] ){
			std::sort(this->CustomLeftovers[ii].begin(),this->CustomLeftovers[ii].end());
		}
	}
	// Add events from each module spill to the RawEvents vector
	for( size_t ii = 0; ii < this->CustomLeftovers.size(); ++ii ){
		auto evt = this->CustomLeftovers[ii].front();
		auto spill = LeftoverSpillIDs[ii].front();
		while(true){
			if( this->CustomLeftovers[ii].empty() ){
				break;
			}
			RawEvents->push_back(evt);
			CustomLeftovers[ii].pop_front();
			evt = this->CustomLeftovers[ii].front();

			this->EvtSpillCounter[spill%this->NUMCONCURRENTSPILLS] -= 1;
			LeftoverSpillIDs[ii].pop_front();
			spill = LeftoverSpillIDs[ii].front();
		}
	}
	// Re-sort the RawEvents vector
	if( RawEvents->size() > 0 ){
		std::sort(RawEvents->begin(),RawEvents->end());
	}
	return Translator::TRANSLATORSTATE::COMPLETE;
}

int LDFPixieTranslator::ParseDirBuffer(){
	// With the current file, check the buffer type and make sure it matches the DIR buffer type
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->check_bufftype)),sizeof(uint32_t));
	if ( this->check_bufftype != this->CurrDirBuff.dirBuffType ){
		this->console->warn("Invalid DIR buffer type");
		this->CurrentFile.seekg(-1*sizeof(uint32_t),this->CurrentFile.cur);
		return -1;
	}	
	// Read the buffer size and check it matches the expected size
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->check_buffsize)),sizeof(uint32_t));
	if( this->check_buffsize != this->CurrDirBuff.dirBufferSize ){
		this->console->warn("Invalid DIR buffer size");
		this->CurrentFile.seekg(-2*sizeof(uint32_t),this->CurrentFile.cur);
		return -1;
	}
	// Read the file buffer size. This should be the same as the difBufferSize + 2.
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->check_filebuffsize)),sizeof(uint32_t));
	if( this->check_filebuffsize != this->CurrDirBuff.fileBufferSize ){
		this->console->warn("Invalid File buffer size");
		this->CurrentFile.seekg(-3*sizeof(uint32_t),this->CurrentFile.cur);
		return -1;
	}
	// Get the total buffers in the file. EOF should be at totalFileBuffers * fileBufferSize * sizeof(uint32_t) from the start of the file.
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDirBuff.totalFileBuffers)),sizeof(uint32_t));
	// Not sure with these are for - JH
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDirBuff.unknown)),2*(sizeof(uint32_t)));
	// Read the run number from the directory buffer
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDirBuff.run_num)),sizeof(uint32_t));
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDirBuff.unknown[2])),sizeof(uint32_t));
	// Seek to head buffer position -> This is 4x buffers from file start
	++this->buffersRead;
	this->CurrentFile.seekg(this->CurrDirBuff.fileBufferSize*sizeof(uint32_t)*buffersRead, this->CurrentFile.beg);
	this->console->info("Parsed Dir Buffer");
	this->console->info("found total buff size : {}",this->CurrDirBuff.totalFileBuffers);
	this->console->info("unknown [0-2] : {} {} {}",this->CurrDirBuff.unknown[0],this->CurrDirBuff.unknown[1],this->CurrDirBuff.unknown[2]);
	this->console->info("runnum : {}",this->CurrDirBuff.run_num);
	return 0;
}

int LDFPixieTranslator::ParseHeadBuffer(){
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->check_bufftype)),sizeof(uint32_t));
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->check_buffsize)),sizeof(uint32_t));
	if( this->check_bufftype != this->CurrHeadBuff.bufftype or this->check_buffsize != this->CurrHeadBuff.buffsize ){
		this->console->warn("Invalid HEAD buffer");
		this->CurrentFile.seekg(-8,this->CurrentFile.cur);
		return -1;
	}	
	std::streamsize headOffset = 0;
	// Get facility name, format name, type name, date, run title, and run number, and add null terminaltion to strings.
	// Get facility name (8 characters)
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrHeadBuff.facility)),8);
	this->CurrHeadBuff.facility[8] = '\0';
	headOffset += this->CurrentFile.gcount();
	// Get format name (8 characters)
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrHeadBuff.format)),8);
	this->CurrHeadBuff.format[8] = '\0';
	headOffset += this->CurrentFile.gcount();
	// Get type name (16 characters)
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrHeadBuff.type)),16);
	this->CurrHeadBuff.type[16] = '\0';
	headOffset += this->CurrentFile.gcount();
 // Get date (16 characters)
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrHeadBuff.date)),16);
	this->CurrHeadBuff.date[16] = '\0';
	headOffset += this->CurrentFile.gcount();
  // Get run title (80 characters)
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrHeadBuff.run_title)),80);
	this->CurrHeadBuff.run_title[80] = '\0';
	headOffset += this->CurrentFile.gcount();
	// Get run number (4 bytes)
	this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrHeadBuff.run_num)),sizeof(uint32_t));
	headOffset += this->CurrentFile.gcount();
	// Seek to the next buffer. This is the first data buffer.
	++this->buffersRead;
	this->CurrentFile.seekg(this->CurrDirBuff.fileBufferSize*sizeof(uint32_t)*buffersRead, this->CurrentFile.beg);
	this->console->info("Found Head Buffer");
	this->console->info("facility : {}",this->CurrHeadBuff.facility);
	this->console->info("format : {}",this->CurrHeadBuff.format);
	this->console->info("type : {}",this->CurrHeadBuff.type);
	this->console->info("date : {}",this->CurrHeadBuff.date);
	this->console->info("title : {}",this->CurrHeadBuff.run_title);
	this->console->info("run : {}",this->CurrHeadBuff.run_num);

	return 0;
}

// ParseDataBuffer reads all data buffers from the current file until it reaches the double end of file (EOF) buffers
int LDFPixieTranslator::ParseDataBuffer(uint32_t& nBytes,bool& full_spill,bool& bad_spill){
	bool first_chunk = true;
	bad_spill = false;
	uint32_t this_chunk_sizeB;
	uint32_t total_num_chunks = 0;
	uint32_t current_chunk_num = 0;
	uint32_t prev_chunk_num;
	uint32_t prev_num_chunks;
	nBytes = 0;

	while( true ){
		if( this->ReadNextBuffer() == -1 and (this->CurrDataBuff.buffhead != HRIBF_TYPES::ENDFILE) ){
			this->console->critical("Failed to read from input data file");
			// Return if we failed to read the next buffer
			return 6;
		}
		
		// If we reach the first EOF buffer, we should check if the next buffer is also EOF
		if( this->CurrDataBuff.buffhead == HRIBF_TYPES::ENDFILE ){
			if( this->CurrDataBuff.nextbuffhead == HRIBF_TYPES::ENDFILE ){
				this->console->info("Read double EOF");
				// End of file, this should be the goal.
				return 2;
			}else{
				this->console->info("Reached single EOF, force reading next");
				this->ReadNextBuffer(true);
				return 1;
			}

		// If we are not at the first EOF buffer, process as a normal DATA buffer
		}else if( this->CurrDataBuff.buffhead == HRIBF_TYPES::DATA ){
			prev_chunk_num = current_chunk_num;
			prev_num_chunks = total_num_chunks;
			// Get the total number of bytes in the current data buffer (should be )
			this_chunk_sizeB = this->CurrDataBuff.currbuffer->at(this->CurrDataBuff.buffpos++);
			total_num_chunks = this->CurrDataBuff.currbuffer->at(this->CurrDataBuff.buffpos++);
			current_chunk_num = this->CurrDataBuff.currbuffer->at(this->CurrDataBuff.buffpos++);

			if( first_chunk ){
				if( current_chunk_num != 0 ){
					this->console->critical("first chunk {} isn't chunk 0 at spill {}",current_chunk_num,this->CurrSpillID);
					this->CurrDataBuff.missingchunks += current_chunk_num;
					full_spill = false;
				}else{
					full_spill = true;
				}
				first_chunk = false;
			}else if( total_num_chunks != prev_num_chunks ){
				this->console->critical("Gotten out of order parsing spill {}",this->CurrSpillID);
				this->ReadNextBuffer(true);
				this->CurrDataBuff.missingchunks += (prev_num_chunks - 1) - prev_chunk_num;
				return 4; 
			}else if( current_chunk_num != prev_chunk_num+1 ){
				full_spill = false;
				if( current_chunk_num == prev_chunk_num+2 ){
					this->console->critical("Missing single spill chunk {} at spill {}",prev_chunk_num+1,this->CurrSpillID);
				}else{
					this->console->critical("Missing multiple spill chunks from {} to {} at spill {}",prev_chunk_num+1,current_chunk_num-1,this->CurrSpillID);
				}
				this->ReadNextBuffer(true);
				this->CurrDataBuff.missingchunks += std::abs(static_cast<double>(static_cast<double>(current_chunk_num - 1) - prev_chunk_num));
				return 4;
			}

			if( current_chunk_num == total_num_chunks - 1) {//spill footer
				if( this_chunk_sizeB != 20 ){
					this->console->critical("spill footer (chunk {} of {}) has size {} != 5 at spill {}",current_chunk_num,total_num_chunks,this_chunk_sizeB,this->CurrSpillID);
					this->ReadNextBuffer(true);
					return 5;
				}
				//memcpy(&data_[nBytes],&curr_buffer[buff_pos],8)
				uint32_t nWords = 2;
				for( uint32_t ii = 0; ii < nWords; ++ii ){
					this->databuffer.push_back(this->CurrDataBuff[this->CurrDataBuff.buffpos+ii]);
				}
				nBytes += 8;
				this->CurrDataBuff.buffpos += 2;
				return 0;
			}else{
				uint32_t copied_bytes = 0;
				//this is what it is in utkscan
				//if( this_chunk_sizeB < =12 ){
				//the fix seems just as simple as ignoring the == 12 case
				//this does warrant investigation though
				//would have to compare against uktscanor output though
				//is probably fine though
				if( this_chunk_sizeB < 12 ){
					this->console->critical("invalid number of bytes in chunk {} of {}, {} bytes at spill {}",current_chunk_num+1,total_num_chunks,this_chunk_sizeB,this->CurrSpillID);
					++this->CurrDataBuff.missingchunks;
					return 4;
				}
				++this->CurrDataBuff.goodchunks;
				copied_bytes = this_chunk_sizeB - 12;
				//memcpy(&data_[nBytes],&curr_buffer[buff_pos],copied_bytes);
				uint32_t nWords = copied_bytes/4;
				for( uint32_t ii = 0; ii < nWords; ++ii ){
					this->databuffer.push_back(this->CurrDataBuff[this->CurrDataBuff.buffpos+ii]);
				}
				nBytes += copied_bytes;
				this->CurrDataBuff.buffpos += copied_bytes/4;
			}

		// This was not a DATA buffer or an EOF buffer, so something went wrong, force rotate buffers.
		}else{
			this->console->critical("found non data/non eof buffer 0x{:x}",this->CurrDataBuff.buffhead);
			this->ReadNextBuffer(true);
		}
	}
	return 0;
}

int LDFPixieTranslator::ReadNextBuffer(bool force){
	if( this->CurrDataBuff.bcount == 0 ){
		// This seems super jank... really trying to read the curren data buffer into a vector of unsigned ints.
		this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDataBuff.buffer1[0])),this->CurrDirBuff.fileBufferSize*sizeof(uint32_t));
	}else if( this->CurrDataBuff.buffpos + 3 < this->CurrDirBuff.fileBufferSize and not force ){
		while( this->CurrDataBuff.currbuffer->at(this->CurrDataBuff.buffpos) == HRIBF_TYPES::ENDBUFF and  this->CurrDataBuff.buffpos < 8193 ){
			++(this->CurrDataBuff.buffpos);
		}
		if( this->CurrDataBuff.buffpos + 3 < 8193 ){
			return 0;
		}
	}
	if( this->CurrDataBuff.bcount % 2 == 0 ){
		this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDataBuff.buffer2[0])),this->CurrDirBuff.fileBufferSize*sizeof(uint32_t));
		this->CurrDataBuff.currbuffer = &(this->CurrDataBuff.buffer1);
		this->CurrDataBuff.nextbuffer = &(this->CurrDataBuff.buffer2);
	}else{
		this->CurrentFile.read(reinterpret_cast<char*>(&(this->CurrDataBuff.buffer1[0])),this->CurrDirBuff.fileBufferSize*sizeof(uint32_t));
		this->CurrDataBuff.currbuffer = &(this->CurrDataBuff.buffer2);
		this->CurrDataBuff.nextbuffer = &(this->CurrDataBuff.buffer1);
	}
	++(this->CurrDataBuff.bcount);
	this->CurrDataBuff.buffpos = 0;
	this->CurrDataBuff.buffhead = this->CurrDataBuff.currbuffer->at(this->CurrDataBuff.buffpos++);
	this->CurrDataBuff.buffsize = this->CurrDataBuff.currbuffer->at(this->CurrDataBuff.buffpos++);
	
	this->CurrDataBuff.nextbuffhead = this->CurrDataBuff.nextbuffer->at(0);
	this->CurrDataBuff.nextbuffsize = this->CurrDataBuff.nextbuffer->at(1);
	if( not this->CurrentFile.good() ){
		return -1;
	}else if( this->CurrentFile.eof() ){
		return 2;
	}

	return 0;
}

// UnpackData for the current spill
int LDFPixieTranslator::UnpackData(uint32_t& nBytes,bool& full_spill,bool& bad_spill,std::vector<bool>& entriesread, std::vector<DDASRootHit>* RawEvents){
	uint32_t nWords = nBytes/4;
	uint32_t nWords_read = 0;
	uint32_t spillLength = 0xFFFFFFFF;
	uint32_t vsn = 0xFFFFFFFF;
	DDASRootHit currentHit;
	ddasfmt::DDASHitUnpacker unpacker;
	uint32_t eventLength = 0;
	// auto currsize = this->Leftovers.size();
	time_t theTime = 0;
	this->NTotalWords += nWords;
	while( nWords_read <= nWords ){
		while( this->databuffer[nWords_read] == 0xFFFFFFFF ){
			++nWords_read;
		}
		spillLength = this->databuffer[nWords_read];
		vsn = this->databuffer[nWords_read + 1];
		// this->console->info("spillLength : {} vsn : {}",spillLength,vsn);

		if( spillLength == 6 ){
			nWords_read += spillLength;
			continue;
		}

		if( vsn < 14 ){
			//this->console->info("{} {}",spillLength,vsn);
			if( spillLength == 2 ){
				nWords_read += spillLength;
				continue;
			}else{
				//good module readout
				uint32_t buffpos = nWords_read+2; 
				while( buffpos < (nWords_read + spillLength) ){

					// UNPACKING DATA HERE!!!
					// Use the first word to get the module number, crate number
					AddDDASWords(buffpos, eventLength, spillLength, entriesread);

					firstWords = &(this->databuffer[buffpos]);

					currentHit.Reset();
					const uint32_t* finish = unpacker.unpack(firstWords, firstWords+eventLength, currentHit);
					if (finish-firstWords != eventLength) {
						this->console->error("Unpacked event length {} does not match expected length {}", finish-firstWords, eventLength);
						throw std::runtime_error("Unpacked event length does not match expected length");
					}
					buffpos += eventLength;
					CustomLeftovers[currentHit.getSlotID()].push_back(currentHit);
					this->LeftoverSpillIDs[currentHit.getSlotID()].push_back(this->CurrSpillID);

					++this->EvtSpillCounter[this->CurrSpillID%this->NUMCONCURRENTSPILLS];
				}
				nWords_read += spillLength;
			}
		}else if( vsn == 1000 ){
			//this is for superheavy
			memcpy(&theTime,&(this->databuffer[nWords_read + 2]),sizeof(time_t));
			this->console->info("ctime : {}",ctime(&theTime));
			nWords_read += spillLength;
			continue;
			// End of spill
		}else if( vsn == 9999 ){
			//end of readout
			//auto finalsize = this->Leftovers.size();
			//this->EvtSpillCounter[this->CurrSpillID%this->NUMCONCURRENTSPILLS] = (finalsize - currsize);
			//this->console->info("evts added {}",(finalsize-currsize));
			//this->console->info("spill : {} words : {} Total words : {}",this->CurrSpillID,nWords,this->NTotalWords);
			++(this->CurrSpillID);
			this->databuffer.clear();
			break;
		}else{
			++(this->CurrSpillID);
			this->databuffer.clear();
			this->console->critical("UNEXPECTED VSN : {}",vsn);
			break;
		}
	}
	return 0;
}

int LDFPixieTranslator::CountBuffersWithData() const{
	//return std::count_if(this->EvtSpillCounter.begin(),this->EvtSpillCounter.end(),[](const int& x){ return x > 0; });
	int numspill = 0;
	//for( const auto& itr : this->EvtSpillCounter ){
	for( size_t ii = 0; ii < this->EvtSpillCounter.size(); ++ii ){
		numspill += (this->EvtSpillCounter[ii] > 0);
		//numspill += (itr > 0);
		//if( itr > 0 ){
		//	++numspill;
		//}
	}
	return numspill;
	//if( this->EvtSpillCounter.size() == 0 ){
	//	return 0;
	//}else{
	//	int numspill = 0;
	//	for( const auto& kv : this->EvtSpillCounter ){
	//		if( kv.second > 0 ){
	//			++numspill;
	//			//this->console->info("spill {} : entries {}",kv.first,kv.second);
	//		}
	//	}
	//	//this->console->info("done");
	//	return numspill;
	//}
}

void LDFPixieTranslator::AddDDASWords(const uint32_t&buffpos, uint32_t&eventLength, uint32_t&spillLength, std::vector<bool>& entriesread){
	const uint32_t firstWord = this->databuffer[buffpos];
	
	eventLength = ((firstWord & 0x3FFE0000)>>17) + 2;
	uint32_t DDASWord1 = eventLength*2;

	std::pair<uint32_t, uint32_t> moduleCratePair(((firstWord & 0x00000F00) >> 8), ((firstWord & 0x000000F0) >> 4));
	entriesread[moduleCratePair.second-2] = true;
	
	// get modArray which has the msps, ADC resolution, and the hardware revision.
	const std::array<uint32_t, 3> modArray = CmdOpts.mod_params_map[moduleCratePair];
	uint32_t DDASWord2 = (modArray[0] & 0xFFFF)|((modArray[1]<<16)&0x00FF0000)|((modArray[2]<<24)&0xFF000000);

	this->databuffer.insert(this->databuffer.begin() + buffpos, DDASWord1);
	this->databuffer.insert(this->databuffer.begin() + buffpos + 1, DDASWord2);
	spillLength += 2;

}
