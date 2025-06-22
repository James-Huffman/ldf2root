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
 * @file DDASHit.h
 * @brief DDASHit class definition. 
 */

#ifndef DDASHIT_H
#define DDASHIT_H

#include <stdint.h>

#include <vector>

/** @namespace ddasfmt */
namespace ddasfmt {
	
    /**
     * @addtogroup ddasfmt libDDASFormat.so
     * @brief DDAS data format library.
     *
     * @details
     * This library contains the DDASHitUnpacker, a class to unpack 
     * Pixie-16 data recorded by FRIBDAQ into a generic, module-independent
     * format defined by the DDASHit class. The DDASHit class comes with a
     * collection of getter and setter functions to access and manipulate 
     * data.
     * @{
     */
	
    /**
     * @class DDASHit
     * 
     * @brief Encapsulation of a generic DDAS event.
     *
     * @details
     * The DDASHit class is intended to encapsulate the information that
     * is emitted by the Pixie-16 digitizer for a single event. It contains
     * information for a single channel only. It is generic because it can
     * store data for the 100 MSPS, 250 MSPS, and 500 MSPS Pixie-16 
     * digitizers used at the lab. In general all of these contain the 
     * same set of information, however, the meaning of the CFD data is 
     * different for each. The DDASHit class abstracts these differences 
     * away from the user.
     *
     * This class does not provide any parsing capabilities likes its
     * companion class ddasdumper. To fill this with data, you should use 
     * the DDASHitUnpacker class. Here is how you use it:
     *
     * @code
     * DDASHit hit;
     * DDASHitUnpacker unpacker;
     * unpacker.unpack(pData, pData + sizeOfHit, hit);
     * @endcode
     *
     * where `pData` is a pointer to the first 32-bit word of the event 
     * and `sizeOfHit` is the size also in 32-bit words. Note that 
     * `pData + sizeOfHit` is a pointer off the end of the current hit.
     */
    class DDASHit {
	    
    private:
	    
	// Channel events always have the following info:
	    
	double   m_time;          //!< Assembled time including CFD.
	uint64_t m_coarseTime;    //!< Assembled time without CFD.
	uint64_t m_externalTimestamp; //!< External timestamp.
	uint32_t m_timeHigh;      //!< Bits 32-47 of timestamp.
	uint32_t m_timeLow;       //!< Bits 0-31 of timestamp.
	uint32_t m_timeCFD;       //!< Raw cfd time.
	uint32_t m_energy;        //!< Energy of event.
	uint32_t m_finishCode;    //!< Indicates whether pile-up occurred.
	uint32_t m_channelLength; //!< Number of 32-bit words of raw data.
	uint32_t m_channelHeaderLength; //!< Length of header.
	uint32_t m_chanID;        //!< Channel index.
	uint32_t m_slotID;        //!< Slot index.
	uint32_t m_crateID;       //!< Crate index.
	uint32_t m_cfdTrigSourceBit; //!< ADC clock cycle for CFD ZCP.
	uint32_t m_cfdFailBit;    //!< Indicates whether the CFD failed.
	uint32_t m_traceLength;   //!< Length of stored trace.
	uint32_t m_modMSPS;       //!< Sampling rate of the module (MSPS).
	int      m_hdwrRevision;  //!< Hardware revision.
	int      m_adcResolution; //!< ADC resolution.
	bool     m_adcOverflowUnderflow; //!< =1 if over- or under-flow.

	// Storage for extra data which may be present in a hit:
	    
	std::vector<uint32_t> m_energySums; //!< Energy sum data.
	std::vector<uint32_t> m_qdcSums;    //!< QDC sum data.
	std::vector<uint16_t> m_trace;      //!< Trace data.
	    
    public:
	/** @brief Default constructor. */
	DDASHit();
	    
    private:
	/**
	 * @brief Copy in data from another DDASHit.
	 * @param rhs Reference to the DDASHit to copy.
	 */
	void copyIn(const DDASHit& rhs);
	    
    public:      
	/** @brief Copy constructor */
	DDASHit(const DDASHit& obj) {
	    copyIn(obj);
	}
	/** @brief Assignment operator */
	DDASHit& operator=(const DDASHit& obj) {
	    if (this != &obj) {
		copyIn(obj);
	    }
	    return *this;
	}

		bool operator<(const DDASHit&) const;
		bool operator>(const DDASHit&) const;
		bool operator<=(const DDASHit&) const;
		bool operator>=(const DDASHit&) const;
		bool operator==(const DDASHit&) const;
		bool operator!=(const DDASHit&) const;


	/** 
	 * @brief Destructor. 
	 * @details
	 * The destrutor is virtual to ensure proper destruction of 
	 * objects derived from DDASHit.
	 */
	virtual ~DDASHit();
	/** 
	 * @brief Resets the state of all member data to that of 
	 * initialization
	 */
	void Reset();

	/** 
	 * @brief Retrieve computed time 
	 * @details
	 * This method performs a computation that depends on the type of 
	 * the digitizer that produced the data. In each case, the coarse 
	 * timestamp is formed using the timelow and timehigh. This is 
	 * coarse timestamp is then corrected using any CFD time that 
	 * exists.
	 *
	 * The calculations for the various modules are as follows:
	 *
	 * For the 100 MSPS modules:
	 *
	 * \f[\text{time} = 10\times((\text{timeHigh} << 32) 
	 * + \text{timeLow} + \text{timeCFD}/2^{15})\f]
	 *  
	 * For the 250 MSPS modules:
	 *
	 * \f[\text{time} = 8\times((\text{timeHigh} << 32) 
	 * + \text{timeLow}) + 4\times(\text{timeCFD}/2^{14}
	 * - \text{cfdTrigSourceBit})\f]
	 *
	 * For the 500 MSPS modules:
	 *
	 * \f[\text{time} = 10\times((\text{timeHigh} << 32) 
	 * + \text{timeLow}) + 2\times(\text{timeCFD}/2^{13}
	 * + \text{cfdTrigSourceBit} - 1)\f]
	 *
	 * @return double The timestamp in units of nanoseconds.
	 */
	double getTime() const { return m_time; }
	/** 
	 * @brief Retrieve the raw timestamp in nanoseconds without 
	 * any CFD correction.
	 * @details
	 * Latching of the coarse timestamp depends on whether or not 
	 * the CFD is enabled, and, if enabled, whether the CFD algorithm 
	 * succeeds or not:
	 * - If the CFD is enabled and a vaild CFD exists, the coarse 
	 *   timestamp is latched to the trace sample immidiately prior 
	 *   to the zero-crossing point.
	 * - If the CFD is enabled and fails, the coarse timestamp is 
	 *   latched to the leading-edge trigger point.
	 * - If the CFD is disabled, the coarse timestamp is latched to 
	 *   the leading-edge trigger point.
	 * @return The raw timestamp in nanoseconds. 
	 */
	uint64_t getCoarseTime() const { return m_coarseTime; }
	/** 
	 * @brief Retrieve the energy.
	 * @details
	 * With the advent of Pixie-16 modules with 16-bit ADCs, the 
	 * `getEnergy()` method no longer includes the ADC 
	 * overflow/underflow bit. The overflow/underflow bit can be 
	 * accessed via the `getADCOverflowUnderflow()` method instead.
	 * @return The energy in ADC units.
	 */
	uint32_t getEnergy() const { return m_energy; }	    
	/** 
	 * @brief Retrieve most significant 16-bits of raw timestamp.
	 * @return The upper 16 bits of the 48-bit timestamp. 
	 */
	uint32_t getTimeHigh() const { return m_timeHigh; }	    
	/** 
	 * @brief Retrieve least significant 32-bit of raw timestamp.
	 * @return The lower 32 bits of the 48-bit timestamp. 
	 */
	uint32_t getTimeLow() const { return m_timeLow; }	    
	/**
	 * @brief Retrieve the raw CFD time.
	 * @return The raw CFD time value from the data word. 
	 */
	uint32_t getTimeCFD() const { return m_timeCFD; }	    
	/** 
	 * @brief Retrieve finish code
	 * @return The finish code.
	 * @details
	 * The finish code will be set to 1 if pileup was detected.
	 */
	uint32_t getFinishCode() const { return m_finishCode; }	    
	/** 
	 * @brief Retrieve number of 32-bit words that were in original 
	 * data packet.
	 * @return The number of 32-bit words in the event.
	 * @details
	 * Note that this only really makes sense to be used if the object 
	 * was filled with data using UnpackChannelData().
	 */
	uint32_t getChannelLength() const { return m_channelLength; }	    
	/** 
	 * @brief Retrieve length of header in original data packet. 
	 * @return Length of the channel header. 
	 */
	uint32_t getChannelHeaderLength() const {
	    return m_channelHeaderLength;
	}	    	    
	/** 
	 * @brief Retrieve the slot that the module resided in. 
	 * @return Module slot. 
	 */
	uint32_t getSlotID() const { return m_slotID; }	    
	/** 
	 * @brief Retrieve the index of the crate the module resided in. 
	 * @return Module crate ID. 
	 */
	uint32_t getCrateID() const { return m_crateID; }	    
	/** 
	 * @brief Retrieve the channel index. 
	 * @return Channel index on the module. 
	 */
	uint32_t getChannelID() const { return m_chanID; }	    
	/** 
	 * @brief Retrieve the ADC frequency of the module. 
	 * @return Module ADC MSPS. 
	 */
	uint32_t getModMSPS() const { return m_modMSPS; }	    
	/** 
	 * @brief Retrieve the hardware revision. 
	 * @return int  Module hardware revision number. 
	 */
	int getHardwareRevision() const { return m_hdwrRevision; }	    
	/** 
	 * @brief Retrieve the ADC resolution.
	 * @return Module ADC resolution (bit depth). 
	 */
	int getADCResolution() const { return m_adcResolution; }	    
	/** 
	 * @brief Retrieve trigger source bit from CFD data. 
	 * @return The CFD trigger source bit.
	 */
	uint32_t getCFDTrigSource()	const { return m_cfdTrigSourceBit; }
	/** 
	 * @brief Retreive failure bit from CFD data.
	 * @return The CFD fail bit.
	 * @details
	 * The fail bit == 1 if the CFD fails, 0 otherwise.
	 */
	uint32_t getCFDFailBit() const { return m_cfdFailBit; }	    
	/**
	 * @brief Retrieve trace length 
	 * @return The trace length in ADC samples.
	 */
	uint32_t getTraceLength() const { return m_traceLength; }	    
	/** 
	 * @brief Access the trace data 
	 * @return The ADC trace.
	 */
	std::vector<uint16_t>& getTrace() { return m_trace; }
	/** 
	 * @brief Access the trace data 
	 * @return The ADC trace.
	 */
	const std::vector<uint16_t>& getTrace() const { return m_trace; }
	/** 
	 * @brief Access the energy/baseline sum data.
	 * @return The energy sum data.
	 */
	std::vector<uint32_t>& getEnergySums() { return m_energySums; }
	/** 
	 * @brief Access the energy/baseline sum data.
	 * @return The energy sum data.
	 */
	const std::vector<uint32_t>& getEnergySums() const {
	    return m_energySums;
	}	    
	/** 
	 * @brief Access the QDC data.
	 * @return The QDC sum data.
	 */
	std::vector<uint32_t>& getQDCSums() { return m_qdcSums; }	    
	/** 
	 * @brief Access the QDC data.
	 * @return The QDC sum data.
	 */
	const std::vector<uint32_t>& getQDCSums() const {
	    return m_qdcSums;
	}
	/**
	 * @brief Retrieve the external timestamp.
	 * @return The 48-bit external timestamp in clock ticks.
	 */
	uint64_t getExternalTimestamp() const {
	    return m_externalTimestamp;
	}
	/** 
	 * @brief Retrieve the ADC overflow/underflow status
	 * @return bool
	 * @retval true  If the ADC over- or underflows.
	 * @retval false Otherwise.
	 * @details
	 * In the 12 and 14 bit modules, this is the value of bit 15 in 
	 * the 4th header word. In the 16 bit modules, this is the value 
	 * of bit 31 in the 4th header word.
	 */
	bool getADCOverflowUnderflow() const {
	    return m_adcOverflowUnderflow;
	}

	/**
	 * @brief Set the channel ID.
	 * @param channel Channel value for this hit.
	 */
	void setChannelID(uint32_t channel);
	/**
	 * @brief Set the slot ID.
	 * @param slot Slot value for this hit.
	 */
	void setSlotID(uint32_t slot);
	/**
	 * @brief Set the crate ID.
	 * @param crate Crate ID value for this hit.
	 */
	void setCrateID(uint32_t crate);
	/**
	 * @brief Set the channel header length
	 * @param channelHeaderLength Channel header length of this hit.
	 */
	void setChannelHeaderLength(uint32_t channelHeaderLength);	    
	/**
	 * @brief Set the channel length.
	 * @param channelLength The length of the hit.
	 */
	void setChannelLength(uint32_t channelLength);
	/**
	 * @brief Set the finish code.
	 * @param finishCode Finish code for this hit.
	 */
	void setFinishCode(bool finishCode);
	/**
	 * @brief Set the coarse timestamp.
	 * @param time The coarse timestamp.
	 */
	void setCoarseTime(uint64_t time);
	/**
	 * @brief Set the raw CFD time.
	 * @param data The raw CFD value from the data word.
	 */
	void setRawCFDTime(uint32_t data);
	/**
	 * @brief Set the CFD trigger source bit.
	 * @param bit The CFD trigger source bit value for this hit.
	 */
	void setCFDTrigSourceBit(uint32_t bit);
	/**
	 * @brief Set the CFD fail bit.
	 * @param bit The CFD fail bit value.
	 */
	void setCFDFailBit(uint32_t bit);
	/**
	 * @brief Set the lower 32 bits of the 48-bit timestamp.
	 * @param datum  The lower 32 bits of the timestamp.
	 */
	void setTimeLow(uint32_t datum);
	/**
	 * @brief Set the higher 16 bits of the 48-bit timestamp.
	 * @param datum The higher 16 bits of the 48-bit timestamp 
	 *   extracted from the lower 16 bits of the 32-bit word passed to
	 *   this function.
	 */
	void setTimeHigh(uint32_t datum);
	/**
	 * @brief Set the hit time.
	 * @param compTime The computed time for this hit with the CFD 
	 *   correction applied.
	 */
	void setTime(double compTime);
	/**
	 * @brief Set the energy for this hit.
	 * @param energy The energy for this hit.
	 */
	void setEnergy(uint32_t energy);
	/**
	 * @brief Set the ADC trace length.
	 * @param length The length of the trace in 16-bit words (samples).
	 */
	void setTraceLength(uint32_t length);
	/**
	 * @brief Set the value of the ADC frequency in MSPS for the ADC
	 * which recorded this hit.
	 * @param msps The ADC frequency in MSPS.
	 */
	void setModMSPS(uint32_t msps);
	/**
	 * @brief Set the value of the ADC resolution (bit depth) for the 
	 * ADC which recorded this hit.
	 * @param value The ADC resolution.
	 */
	void setADCResolution(int value);
	/**
	 * @brief Set the ADC hardware revision for the ADC which recorded 
	 * this hit.
	 * @param value The hardware revision of the ADC.
	 */
	void setHardwareRevision(int value);
	/**
	 * @brief Set the crate ID.
	 * @param value Crate ID value for this hit.
	 */
	void appendEnergySum(uint32_t value);
	/**
	 * @brief Set the energy sum data from an existing set of sums.
	 * @param eneSums Vector of energy sums.
	 */
	void setEnergySums(std::vector<uint32_t> eneSums);
	/**
	 * @brief Append a QDC value to the vector of QDC sums.
	 * @param value  The QDC value appended to the vector.
	 */
	void appendQDCSum(uint32_t value);
	/**
	 * @brief Set the QDC sum data from an existing set of sums.
	 * @param qdcSums Vector of QDC sums.
	 */
	void setQDCSums(std::vector<uint32_t> qdcSums);
	/**
	 * @brief Append a 16-bit ADC trace sample to the trace vector.
	 * @param value The 16-bit ADC sample appended to the vector.
	 */
	void appendTraceSample(uint16_t value);
	/**
	 * @brief Set the trace data from an existing trace.
	 * @param trace The trace.
	 */
	void setTrace(std::vector<uint16_t> trace);
	/**
	 * @brief Set the value of the external timestamp.
	 * @param value The value of the external timestamp supplied 
	 *   to DDAS. in clock ticks.
	 */
	void setExternalTimestamp(uint64_t value);
	/**
	 * @brief Set ADC over- or under-flow state.
	 * @param state The ADC under-/overflow state. True if the ADC 
	 *   under- or overflows the ADC.
	 */	    
	void setADCOverflowUnderflow(bool state);
    };

    /** @} */
	
} // namespace ddasfmt

#endif
