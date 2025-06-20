/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2016.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Jeromy Tompkins
	     Aaron Chester
	     Facility for Rare Isotope Beams
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
 * @file DDASHitUnpacker.cpp
 * @brief Implementation of an unpacker for DDAS data recorded by 
 * NSCLDAQ/FRIBDAQ.
 */

#include "DDASHitUnpacker.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "DDASBitMasks.h"

using namespace ddasfmt;
	
/**
 * @details
 * This expects data from a DDAS readout program. It will parse the entire 
 * body of the event in a manner that is consistent with the data present. 
 * In other words, it uses the sizes of the event encoded in the data to 
 * determine when the parsing is complete.
 *
 * While it parses, it stores the results into the data members of the object 
 * hit. Prior to parsing, all data members are reset to 0 using the Reset() 
 * method. 
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::unpack(
    const uint32_t* beg, const uint32_t* sentinel, DDASHit& hit
    )
{
    if (beg == sentinel) {
	std::stringstream errmsg;
	errmsg << "DDASHitUnpacker::unpack() ";
	errmsg << "Unable to parse empty data buffer.";
	throw std::runtime_error(errmsg.str());
    }

    const uint32_t* data = beg;

    data = parseBodySize(data, sentinel);
    data = parseModuleInfo(hit, data);
    data = parseHeaderWord0(hit, data);
    data = parseHeaderWords1And2(hit, data);
    data = parseHeaderWord3(hit, data);

    // Finished upacking the minimum set of data:
	    
    uint32_t channelHeaderLength = hit.getChannelHeaderLength();
    uint32_t channellength       = hit.getChannelLength();
    size_t   tracelength         = hit.getTraceLength();
	    
    // We may have more data to unpack:
	    
    if(channellength != (channelHeaderLength + tracelength/2)){
	std::stringstream errmsg;
	errmsg << "ERROR: Data corruption: ";
	errmsg << "Inconsistent data lengths found in header ";
	errmsg << "\nChannel length = " << std::setw(8) << channellength;
	errmsg << "\nHeader length  = " << std::setw(8) << channelHeaderLength;
	errmsg << "\nTrace length   = " << std::setw(8) << tracelength;
	throw std::runtime_error(errmsg.str());
    }

    // Longwords per optional enabled data output:
    // External TS: 2
    // Energy sums: 4
    // QDC sums:    8
    // Trace:       ceil(0.5*L*f)
    //   where L = trace length in microseconds, f = module MSPS

    // uint32_t extraWords = channelHeaderLength - SIZE_OF_RAW_EVENT;

    // if (extraWords) {
	// if(extraWords == SIZE_OF_EXT_TS) {
	//     data = extractExternalTimestamp(data, hit);
	
	// } else if(extraWords == SIZE_OF_ENE_SUMS) {
	//     data = extractEnergySums(data, hit);
	
	// } else if (
	//     extraWords == (SIZE_OF_ENE_SUMS + SIZE_OF_EXT_TS)
	//     ) {
	//     data = extractEnergySums(data, hit);
	//     data = extractExternalTimestamp(data, hit);
	
	// } else if(extraWords == SIZE_OF_QDC_SUMS) {
	//     data = extractQDC(data, hit);
	
	// } else if (extraWords == (SIZE_OF_QDC_SUMS + SIZE_OF_EXT_TS)) {
	//     data = extractQDC(data, hit);
	//     data = extractExternalTimestamp(data, hit);
	
	// } else if(extraWords == (SIZE_OF_ENE_SUMS + SIZE_OF_QDC_SUMS)) {
	//     data = extractEnergySums(data, hit);
	//     data = extractQDC(data, hit);
	
	// } else if(
	//     extraWords == (SIZE_OF_ENE_SUMS + SIZE_OF_QDC_SUMS + SIZE_OF_EXT_TS)
	//     ) {
	//     data = extractEnergySums(data, hit);
	//     data = extractQDC(data, hit);
	//     data = extractExternalTimestamp(data, hit);
	// }
    // }

    switch(channelHeaderLength - SIZE_OF_RAW_EVENT) {
        case(0):
            // No extra data, nothing to do.
            break;
        case (SIZE_OF_EXT_TS):
            data = extractExternalTimestamp(data, hit);
            break;
        case(SIZE_OF_ENE_SUMS):
            data = extractEnergySums(data, hit);
            break;
        case(SIZE_OF_ENE_SUMS + SIZE_OF_EXT_TS):
            data = extractEnergySums(data, hit);
            data = extractExternalTimestamp(data, hit);
            break;
        case(SIZE_OF_QDC_SUMS):
            data = extractQDC(data, hit);
            break;
        case(SIZE_OF_QDC_SUMS + SIZE_OF_EXT_TS):
            data = extractQDC(data, hit);
            data = extractExternalTimestamp(data, hit);
            break;
        case(SIZE_OF_ENE_SUMS + SIZE_OF_QDC_SUMS):
            data = extractEnergySums(data, hit);
            data = extractQDC(data, hit);
            break;
        case(SIZE_OF_ENE_SUMS + SIZE_OF_QDC_SUMS + SIZE_OF_EXT_TS):
            data = extractEnergySums(data, hit);
            data = extractQDC(data, hit);
            data = extractExternalTimestamp(data, hit);
            break;
    }
    // If trace length is non zero, unpack the trace data:
    
    if(tracelength != 0) {
	data = parseTraceData(hit, data);
    }

    return data;
}

/**
 * @details
 * This expects data from a DDAS readout program. It will parse the entire 
 * body of the event in a manner that is consistent with the data present. 
 * In other words, it uses the sizes of the event encoded in the data to 
 * determine when the parsing is complete.
 *
 * While it parses, it stores the results into the data members of the object 
 * hit. Prior to parsing, all data members are reset to 0 using the Reset() 
 * method. 
 */
std::tuple<ddasfmt::DDASHit, const uint32_t*>
ddasfmt::DDASHitUnpacker::unpack(
    const uint32_t *beg, const uint32_t* sentinel
    )
{
    DDASHit hit;
    const uint32_t* data = unpack(beg, sentinel, hit);
    return std::make_tuple(hit, data);
}

/**
 * @details
 * The first word of the body passed to this function is the self-inclusive 
 * event size in 16-bit words. 
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::parseBodySize(
    const uint32_t* data, const uint32_t* sentinel
    )
{
    uint32_t nShorts = *data; 
    // Make sure there is enough data to parse
    if (
	(data + nShorts/sizeof(uint16_t) > sentinel)
	&& (sentinel != nullptr)
	) {
	throw std::runtime_error(
	    "DDASHitUnpacker::parseBodySize() found incomplete event data!"
	    );
    }

    return (data+1);
}

/**
 * @details
 * The lower 16 bits encode the ADC frequency, the upper 16 bits encode the 
 * hardware revision and ADC resolution.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::parseModuleInfo(
    DDASHit& hit, const uint32_t* data
    )
{
    uint32_t datum = *data++;
    hit.setModMSPS(datum & LOWER_16_BIT_MASK);
    hit.setADCResolution((datum & ADC_RESOLUTION_MASK) >> ADC_RESOLUTION_SHIFT);
    hit.setHardwareRevision((datum & HW_REVISION_MASK) >> HW_REVISION_SHIFT);

    return data;
}

/**
 * @details
 * Word 0 contains:
 * - Crate/slot/channel information,
 * - The header and channel lengths in 32-bit words,
 * - The module finish code (equals 1 if piled up).
 *
 * @note In previous versions of the Pixie data format, the ADC out-of-range 
 * bit was stored in bit 30 of word 0 and the channel length was extracted 
 * from bits [17:29]. In the current data format, the out-of-range flag has 
 * been moved to word 3, bit 31, and the channel length mask is extracted 
 * from bits [17:30] allowing up to 16383 32-bit words per channel hit.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::parseHeaderWord0(
    DDASHit& hit, const uint32_t* data
    )
{
    uint32_t datum = *data++;
    hit.setChannelID(datum & CHANNEL_ID_MASK);
    hit.setSlotID((datum & SLOT_ID_MASK) >> SLOT_ID_SHIFT);
    hit.setCrateID((datum & CRATE_ID_MASK) >> CRATE_ID_SHIFT);
    hit.setChannelHeaderLength(
	(datum & HEADER_LENGTH_MASK) >> HEADER_LENGTH_SHIFT
	);
    hit.setChannelLength((datum & CHANNEL_LENGTH_MASK) >> CHANNEL_LENGTH_SHIFT);
    hit.setFinishCode((datum & FINISH_CODE_MASK) >> FINISH_CODE_SHIFT);
      
    return data;
}

/**
 * @details
 * Words 1 and 2 contain the timestamp and CFD information. The meaning of the 
 * CFD word depends on the module type. The unpacker abstracts this meaning 
 * away from the user. Note that we know the module type  if the module 
 * identifier word was unpacked before calling this function.
 *
 * Word 1 contains:
 * - The lower 32 bits of the 48-bit timestamp.
 * Word 2 contains:
 * - The upper 16 bits of the 48-bit timestamp,
 * - The CFD result.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::parseHeaderWords1And2(
    DDASHit& hit, const uint32_t* data
    )
{
    uint32_t timeLow      = *data++;
    uint32_t datum1       = *data++;
    uint32_t timeHigh     = datum1 & LOWER_16_BIT_MASK;
    uint32_t adcFrequency = hit.getModMSPS();
    
    uint64_t coarseTime = computeCoarseTime(adcFrequency, timeLow, timeHigh) ;
    double cfdCorrection = parseAndComputeCFD(hit, datum1);

    hit.setTimeLow(timeLow);
    hit.setTimeHigh(timeHigh);
    hit.setCoarseTime(coarseTime); 
    hit.setTime(static_cast<double>(coarseTime) + cfdCorrection);

    return data;
}

/**
 * @details
 * Word 3 contains:
 * - The trace out-of-range (overflow/underflow) flag,
 * - The trace length in samples (16-bit words),
 * - The hit energy.
 * 
 * @note In the current Pixie list mode data format, the ADC out-of-range flag 
 * is stored in word 3, bit 31 rather than word 0, bit 30. See documentation 
 * for `parseHeaderWord0()` for more info.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::parseHeaderWord3(
    DDASHit& hit, const uint32_t* data
    )
{
    hit.setADCOverflowUnderflow(*data >> OUT_OF_RANGE_SHIFT); // Just bit 31.
    hit.setTraceLength((*data & BIT_30_TO_16_MASK) >> 16);
    hit.setEnergy(*data & LOWER_16_BIT_MASK);

    return (data+1);
}

/**
 * @details
 * The 16-bit trace data is stored two samples to one 32-bit word in 
 * little-endian. The data for sample i is stored in the lower 16 bits while 
 * the data for sample i + 1 is stored in the upper 16 bits. For ADCs with less 
 * than 16-bit resolution, those bits are set to 0.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::parseTraceData(
    DDASHit& hit, const uint32_t* data
    )
{
    std::vector<uint16_t>& trace = hit.getTrace();
    size_t tracelength = hit.getTraceLength();
    trace.reserve(tracelength);
    for(size_t i = 0; i < tracelength/2; i++){
	uint32_t datum = *data++;
	trace.push_back(datum & LOWER_16_BIT_MASK);
	trace.push_back((datum & UPPER_16_BIT_MASK) >> 16);
    }

    return data;
}

/**
 * @details
 * The value of the CFD correction depends on the module. Because the module 
 * information is encoded in the data, this function should be called after 
 * `parseModuleInfo()`.
 */
std::tuple<double, uint32_t, uint32_t, uint32_t>
ddasfmt::DDASHitUnpacker::parseAndComputeCFD(uint32_t ModMSPS, uint32_t data)
{

    double correction;
    uint32_t cfdTrigSource, cfdFailBit, timeCFD;

    // Check on the module MSPS and pick the correct CFD unpacking algorithm 
    if(ModMSPS == 100){
	// 100 MSPS modules don't have trigger source bits
	cfdFailBit    = ((data & BIT_31_MASK) >> 31) ; 
	cfdTrigSource = 0;
	timeCFD       = ((data & BIT_30_TO_16_MASK) >> 16);
	correction    = (timeCFD/32768.0)*10.0; // 32768 = 2^15
    }
    else if (ModMSPS == 250) {
	// CFD fail bit in bit 31
	cfdFailBit    = ((data & BIT_31_MASK) >> 31);
	cfdTrigSource = ((data & BIT_30_MASK) >> 30);
	timeCFD       = ((data & BIT_29_TO_16_MASK) >> 16);
	correction    = (timeCFD/16384.0 - cfdTrigSource)*4.0; 
    }
    else if (ModMSPS == 500) {
	// No fail bit in 500 MSPS modules
	cfdTrigSource = ((data & BIT_31_TO_29_MASK) >> 29);
	timeCFD       = ((data & BIT_28_TO_16_MASK) >> 16);
	correction    = (timeCFD/8192.0 + cfdTrigSource - 1)*2.0;
	cfdFailBit    = (cfdTrigSource == 7) ? 1 : 0;
    }

    return std::make_tuple(correction, timeCFD, cfdTrigSource, cfdFailBit);
}

/**
 * @details
 * The value of the CFD correction depends on the module. Because the module 
 * information is encoded in the data, this function should be called after 
 * `parseModuleInfo()`.
 */
double
ddasfmt::DDASHitUnpacker::parseAndComputeCFD(DDASHit& hit, uint32_t data)
{
    double correction;
    uint32_t cfdTrigSource, cfdFailBit, timeCFD;
    uint32_t ModMSPS = hit.getModMSPS();

    // check on the module MSPS and pick the correct CFD unpacking algorithm 
    if(ModMSPS == 100){
	// 100 MSPS modules don't have trigger source bits
	cfdFailBit    = ((data & BIT_31_MASK) >> 31) ; 
	cfdTrigSource = 0;
	timeCFD       = ((data & BIT_30_TO_16_MASK) >> 16);
	correction    = (timeCFD/32768.0)*10.0; // 32768 = 2^15
    }
    else if (ModMSPS == 250) {
	// CFD fail bit in bit 31
	cfdFailBit    = ((data & BIT_31_MASK) >> 31 );
	cfdTrigSource = ((data & BIT_30_MASK) >> 30 );
	timeCFD       = ((data & BIT_29_TO_16_MASK) >> 16);
	correction    = (timeCFD/16384.0 - cfdTrigSource)*4.0; 
    }
    else if (ModMSPS == 500) {
	// no fail bit in 500 MSPS modules
	cfdTrigSource = ((data & BIT_31_TO_29_MASK) >> 29);
	timeCFD       = ((data & BIT_28_TO_16_MASK) >> 16);
	correction    = (timeCFD/8192.0 + cfdTrigSource - 1)*2.0;
	cfdFailBit    = (cfdTrigSource == 7) ? 1 : 0;
    }

    hit.setCFDFailBit(cfdFailBit);
    hit.setCFDTrigSourceBit(cfdTrigSource);
    hit.setRawCFDTime(timeCFD);

    return correction;
}

/** 
 * @details
 * Form the timestamp from the low and high bits and convert it to a time in 
 * nanoseconds.
 *
 * The calculations for the various modules are as follows:
 *
 * For the 100 MSPS module:
 *
 * \f[\text{time} = 10\times((\text{timeHigh} << 32) 
 * + \text{timeLow})\f]
 *  
 * For the 250 MSPS module...
 *
 * \f[\text{time} = 8\times((\text{timeHigh} << 32) 
 * + \text{timeLow})\f]
 *
 * For the 500 MSPS module,
 *
 * \f[\text{time} = 10\times((\text{timeHigh} << 32) 
 * + \text{timeLow})\f]
 */
uint64_t
ddasfmt::DDASHitUnpacker::computeCoarseTime(
    uint32_t adcFrequency, uint32_t timeLow, uint32_t timeHigh
    )
{
    uint64_t tstamp = timeHigh;
    tstamp = tstamp << 32;
    tstamp |= timeLow;

    // Conversion to units of real time depends on module type:
    
    uint64_t toNanoseconds = 10;
    if (adcFrequency == 250) {
	toNanoseconds = 8;
    } 

    return tstamp*toNanoseconds;
}

/**
 * @details
 * Energy sums consist of SIZE_OF_ENE_SUMS (=4) 32-bit words, which are, 
 * in order:
 * 0. The trailing (pre-gap ) sum.
 * 1. The gap sum.
 * 2. The leading (post-gap) sum.
 * 3. The 32-bit IEEE 754 floating point baseline value.
 *
 * If the hit is not reset between calls to this function, the energy sum 
 * data will be appended to the end of the exisiting energy sums.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::extractEnergySums(
    const uint32_t* data, DDASHit& hit
    )
{
    std::vector<uint32_t>& energies = hit.getEnergySums();
    energies.reserve(SIZE_OF_ENE_SUMS);
    energies.insert(energies.end(), data, data + SIZE_OF_ENE_SUMS);
    
    return data + SIZE_OF_ENE_SUMS;
}

/**
 * @details
 * QDC sums consist of SIZE_OF_QDC_SUMS (=8) 32-bit words. If the hit is not 
 * reset between calls to this function, the QDC sum data will be appended 
 * to the end of the exisiting QDC sums.
 */
const uint32_t*
ddasfmt::DDASHitUnpacker::extractQDC(const uint32_t* data, DDASHit& hit)
{
    std::vector<uint32_t>& qdcVals = hit.getQDCSums();
    qdcVals.reserve(SIZE_OF_QDC_SUMS);
    qdcVals.insert(qdcVals.end(), data, data + SIZE_OF_QDC_SUMS);
    
    return data + SIZE_OF_QDC_SUMS;
}

/**
 * @details
 * Unpack and set the 48-bit external timestamp. Unlike the internal timestamp 
 * where the conversion from clock tics to nanoseconds is known, for the 
 * external timestamp no unit conversion is applied. Converting the timestamp 
 * to proper units is left to the user.
 */
const uint32_t* 
ddasfmt::DDASHitUnpacker::extractExternalTimestamp(
    const uint32_t* data, DDASHit& hit
    ) 
{
    uint64_t tstamp = 0;
    uint32_t temp = *data++;
    tstamp = *data++;                 // Lower 32 bits.
    tstamp = ((tstamp << 32) | temp); // Shift upper 32 bits and OR.
    hit.setExternalTimestamp(tstamp);
    
    return data;
}
