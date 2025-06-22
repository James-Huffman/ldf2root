/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2016.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
	     Aaron Chester
	     Facility for Rare Isotope Beams
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
 * @file DDASBitMasks.h
 * @brief Define bit masks to extract data from specific locations of Pixie 
 * data. Bit masks are inclusive: Bits [3:0] means mask bits 3, 2, 1, 0. 
 * Shift offsets are provided for masks where the offset is not obvious from 
 * the mask name.
 */

#ifndef DDASBITMASKS_H
#define DDASBITMASKS_H

#include <cstdint>

/** @namespace ddasfmt */
namespace ddasfmt {

    // Generic masks:

    static const uint32_t LOWER_16_BIT_MASK = 0x0000FFFF; //!< Bits [15:0].
    static const uint32_t BIT_28_TO_16_MASK = 0x1FFF0000; //!< Bits [28:16].
    static const uint32_t BIT_29_TO_16_MASK = 0x3FFF0000; //!< Bits [29:16].
    static const uint32_t BIT_30_TO_16_MASK = 0x7FFF0000; //!< Bits [30:16].
    static const uint32_t BIT_30_TO_29_MASK = 0x60000000; //!< Bits [31:30].
    static const uint32_t BIT_31_TO_29_MASK = 0xE0000000; //!< Bits [31:29].
    static const uint32_t BIT_30_MASK       = 0x40000000; //!< Bit 30.
    static const uint32_t BIT_31_MASK       = 0x80000000; //!< Bit 31.
    static const uint32_t UPPER_16_BIT_MASK = 0xFFFF0000; //!< Bits [31:16].

    // Fragment info added by NSCLDAQ and their offsets:

    static const uint32_t ADC_RESOLUTION_MASK = 0x00FF0000; //!< Bits [23:16].
    static const uint32_t HW_REVISION_MASK    = 0xFF000000; //!< Bits [31:24].

    static const uint32_t ADC_RESOLUTION_SHIFT = 16; //!< ADC resolution offset.
    static const uint32_t HW_REVISION_SHIFT    = 24; //!< HW revision offset.

    // Pixie event data masks and offsets:

    static const uint32_t CHANNEL_ID_MASK     = 0x0000000F;  //!< Bits [3:0].
    static const uint32_t SLOT_ID_MASK        = 0x000000F0;  //!< Bits [7:4].
    static const uint32_t CRATE_ID_MASK       = 0x00000F00;  //!< Bits [11:8].
    static const uint32_t HEADER_LENGTH_MASK  = 0x0001F000;  //!< Bits [16:12].
    static const uint32_t CHANNEL_LENGTH_MASK = 0x7FFE0000;  //!< Bits [30:17].
    static const uint32_t FINISH_CODE_MASK    = BIT_31_MASK; //!< Bit 31.

    static const uint32_t SLOT_ID_SHIFT        = 4;  //!< Slot ID offset.
    static const uint32_t CRATE_ID_SHIFT       = 8;  //!< Crate ID offset.
    static const uint32_t HEADER_LENGTH_SHIFT  = 12; //!< Header length offset.
    static const uint32_t CHANNEL_LENGTH_SHIFT = 17; //!< Chan. length offset.
    static const uint32_t FINISH_CODE_SHIFT    = 31; //!< Finish code offset.
    static const uint32_t OUT_OF_RANGE_SHIFT   = 31; //!< ADC overflow offset.

    // Pixie data sizes:

    static const uint32_t SIZE_OF_RAW_EVENT = 4; //!< 32-bit words event header.
    static const uint32_t SIZE_OF_EXT_TS    = 2; //!< Words for external TS.
    static const uint32_t SIZE_OF_ENE_SUMS  = 4; //!< Words for energy sums.
    static const uint32_t SIZE_OF_QDC_SUMS  = 8; //!< Words for QDC sums.

} // namespace ddasfmt

#endif
