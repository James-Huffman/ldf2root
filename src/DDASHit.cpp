/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2016.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Ron Fox
	     Jeromy Tompkins
	     Aaron Chester
	     Facility for Rare Isotope Beams
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/** 
 * @file DDASHit.cpp
 * @brief Implement DDASHit class used to encapsulate DDAS events.
 */

#include "DDASHit.h"

#include <stdlib.h>

#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "DDASBitMasks.h"

using namespace ddasfmt;

/** 
 * @details
 * All member data are zero-initialized.
 */
ddasfmt::DDASHit::DDASHit() :
    m_time(0), m_coarseTime(0), m_externalTimestamp(0), m_energy(0),
    m_timeHigh(0), m_timeLow(0), m_timeCFD(0), m_finishCode(0),
    m_channelLength(0), m_channelHeaderLength(0),
    m_chanID(0), m_slotID(0), m_crateID(0),
    m_cfdTrigSourceBit(0), m_cfdFailBit(0), m_traceLength(0),
    m_modMSPS(0), m_adcResolution(0), m_hdwrRevision(0),
    m_adcOverflowUnderflow(false),
    m_energySums(), m_qdcSums(), m_trace()
{}

/** 
 * @details
 * For primitive types, this sets the values to 0. For vector 
 * data (i.e. trace), the vector is cleared and resized to 0.
 */
void
ddasfmt::DDASHit::Reset() {
    m_time = 0;
    m_externalTimestamp = 0;
    m_coarseTime = 0;
    m_energy = 0;
    m_timeHigh = 0;
    m_timeLow = 0;
    m_timeCFD = 0;
    m_finishCode = 0;
    m_channelLength = 0;
    m_channelHeaderLength = 0;
    m_chanID = 0;
    m_slotID = 0;
    m_crateID = 0;
    m_cfdTrigSourceBit = 0;
    m_cfdFailBit = 0;
    m_traceLength = 0;
    m_modMSPS = 0;      
    m_adcResolution = 0;
    m_hdwrRevision = 0;
    m_adcOverflowUnderflow = false;
    
    m_energySums.clear();
    m_qdcSums.clear();
    m_trace.clear();    
}
	
/** 
 * @brief Destructor.
 */
ddasfmt::DDASHit::~DDASHit()
{}


bool ddasfmt::DDASHit::operator<(const ddasfmt::DDASHit& rhs) const{
    return this->m_time < rhs.m_time;
}

bool ddasfmt::DDASHit::operator>(const ddasfmt::DDASHit& rhs) const{
    return rhs < (*this);
}

bool ddasfmt::DDASHit::operator<=(const ddasfmt::DDASHit& rhs) const{
    return !((*this) > rhs);
}

bool ddasfmt::DDASHit::operator>=(const ddasfmt::DDASHit& rhs) const{
    return !((*this) < rhs);
}

bool ddasfmt::DDASHit::operator==(const ddasfmt::DDASHit& rhs) const{
    return (this->m_crateID == rhs.m_crateID) and (this->m_slotID == rhs.m_slotID) and (this->m_chanID == rhs.m_chanID) and (this->m_time == rhs.m_time) and (this->m_energy == rhs.m_energy);
}

bool ddasfmt::DDASHit::operator!=(const ddasfmt::DDASHit& rhs) const{
    return !((*this) == rhs);
}

void ddasfmt::DDASHit::setChannelID(uint32_t channel){
    m_chanID = channel;
}

void ddasfmt::DDASHit::setSlotID(uint32_t slot){
    m_slotID = slot;
}

void ddasfmt::DDASHit::setCrateID(uint32_t crate){
    m_crateID = crate;
}

void ddasfmt::DDASHit::setChannelHeaderLength(uint32_t channelHeaderLength){
    m_channelHeaderLength = channelHeaderLength;
}

void ddasfmt::DDASHit::setChannelLength(uint32_t channelLength){
    m_channelLength = channelLength;
}

void ddasfmt::DDASHit::setFinishCode(bool finishCode){
    m_finishCode = finishCode;
}

/**
 * @details
 * Latching of the coarse m_time depends on whether or not the 
 * CFD is enabled, and, if enabled, whether the CFD algorithm 
 * succeeds or not:
 * - If the CFD is enabled and a vaild CFD exists, the coarse 
 *   m_time is latched to the trace sample immidiately prior 
 *   to the zero-crossing point.
 * - If the CFD is enabled and fails, the coarse m_time is 
 *   latched to the leading-edge trigger point.
 * - If the CFD is disabled, the coarse m_time is latched to 
 *   the leading-edge trigger point.
 */
void
ddasfmt::DDASHit::setCoarseTime(uint64_t time)
{
    m_coarseTime = time;
}

void
ddasfmt::DDASHit::setRawCFDTime(uint32_t data)
{
    m_timeCFD = data;
}

/**
 * @details
 * The 250 MSPS and 500 MSPS modules de-serialize data into an FPGA 
 * which operates at some fraction of the ADC sampling rate. The CFD 
 * trigger source bit specifies which fractional time offset from the 
 * FPGA clock tick the CFD zero-crossing occured. For 100 MSPS modules,
 * the source bit is always equal to 0 (FPGA captures data also at
 * 100 MSPS).
 */
void
ddasfmt::DDASHit::setCFDTrigSourceBit(uint32_t bit)
{
    m_cfdTrigSourceBit = bit;
}

/**
 * @details
 * The CFD fail bit == 1 if the CFD algorithm fails. The CFD can fail 
 * if the threshold value is too high or the CFD algorithm fails to 
 * find a zero-crossing point within 32 samples of the leading-edge 
 * trigger point.
 */
void
ddasfmt::DDASHit::setCFDFailBit(uint32_t bit)
{
    m_cfdFailBit = bit;
}

void
ddasfmt::DDASHit::setTimeLow(uint32_t datum)
{
    m_timeLow = datum;
}

void
ddasfmt::DDASHit::setTimeHigh(uint32_t datum)
{
    m_timeHigh = datum & LOWER_16_BIT_MASK;
}

void
ddasfmt::DDASHit::setTime(double compTime)
{
    m_time = compTime;
}

void
ddasfmt::DDASHit::setEnergy(uint32_t energy)
{
    m_energy = energy;
}
	
void
ddasfmt::DDASHit::setTraceLength(uint32_t length)
{
    m_traceLength = length;
}

void
ddasfmt::DDASHit::setModMSPS(uint32_t msps)
{
    m_modMSPS = msps;
}

void
ddasfmt::DDASHit::setADCResolution(int value)
{
    m_adcResolution = value;
}

void
ddasfmt::DDASHit::setHardwareRevision(int value)
{
    m_hdwrRevision = value;
}

void
ddasfmt::DDASHit::appendEnergySum(uint32_t value)
{
    m_energySums.push_back(value);
}

void
ddasfmt::DDASHit::setEnergySums(std::vector<uint32_t> eneSums)
{
    if (eneSums.size() != SIZE_OF_ENE_SUMS) {
	std::string msg("Error setting energy sums: Expected ");
	msg += std::to_string(SIZE_OF_ENE_SUMS);
	msg += " 32-bit words but got ";
	msg += std::to_string(eneSums.size());
	throw std::runtime_error(msg);
    }
    m_energySums = eneSums;
}

void
ddasfmt::DDASHit::appendQDCSum(uint32_t value)
{
    m_qdcSums.push_back(value);
}

void
ddasfmt::DDASHit::setQDCSums(std::vector<uint32_t> qdcSums)
{
    if (qdcSums.size() != SIZE_OF_QDC_SUMS) {
	std::string msg("Error setting QDC sums: Expected ");
	msg += std::to_string(SIZE_OF_QDC_SUMS);
	msg += " 32-bit words but got ";
	msg += std::to_string(qdcSums.size());
	throw std::runtime_error(msg);
    }
    m_qdcSums = qdcSums;
}

void
ddasfmt::DDASHit::appendTraceSample(uint16_t value)
{
    m_trace.push_back(value);
}

void
ddasfmt::DDASHit::setTrace(std::vector<uint16_t> trace)
{
    m_trace = trace;
    setTraceLength(m_trace.size());
}

void
ddasfmt::DDASHit::setExternalTimestamp(uint64_t value)
{
    m_externalTimestamp = value;
}

void
ddasfmt::DDASHit::setADCOverflowUnderflow(bool state)
{
    m_adcOverflowUnderflow = state;
}

///
// Private methods
//

void
ddasfmt::DDASHit::copyIn(const DDASHit& rhs) {
    m_time = rhs.m_time;
    m_externalTimestamp= rhs.m_externalTimestamp;
    m_coarseTime = rhs.m_coarseTime;
    m_energy= rhs.m_energy;
    m_timeHigh = rhs.m_timeHigh;
    m_timeLow = rhs.m_timeLow;
    m_timeCFD = rhs.m_timeCFD;
    m_finishCode = rhs.m_finishCode;
    m_channelLength = rhs.m_channelLength;
    m_channelHeaderLength = rhs.m_channelHeaderLength;
    m_chanID = rhs.m_chanID;
    m_slotID = rhs.m_slotID;
    m_crateID = rhs.m_crateID;
    m_cfdTrigSourceBit = rhs.m_cfdTrigSourceBit;
    m_cfdFailBit = rhs.m_cfdFailBit;
    m_traceLength =rhs.m_traceLength;
    m_modMSPS = rhs.m_modMSPS;
    m_hdwrRevision = rhs.m_hdwrRevision;
    m_adcResolution= rhs.m_adcResolution;
    m_adcOverflowUnderflow = rhs.m_adcOverflowUnderflow;
    
    m_energySums = rhs.m_energySums;
    m_qdcSums =rhs.m_qdcSums;
    m_trace = rhs.m_trace;
}
