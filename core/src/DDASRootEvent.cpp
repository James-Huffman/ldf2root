/*
  This software is Copyright by the Board of Trustees of Michigan
  State University (c) Copyright 2017.

  You may use this software under the terms of the GNU public license
  (GPL).  The terms of this license are described at:

  http://www.gnu.org/licenses/gpl.txt

  Authors:
      Aaron Chester
      Jeromy Tompkins
      FRIB
      Michigan State University
      East Lansing, MI 48824-1321
*/

/**
 * @file DDASRootEvent.cpp
 * @brief Implementation of a class to encapsulate the information in a built
 * DDAS event.
 */ 

#include "DDASRootEvent.h"
#include "DDASRootHit.h"

DDASRootEvent::DDASRootEvent() : TObject(), m_data() {}

/**
 * @details
 * Implements a deep copy.
 */
DDASRootEvent::DDASRootEvent(const DDASRootEvent& obj)
    : TObject(obj), m_data()
{
    // Create new copies of the DDASRootHit events
    for (const auto& hitPtr : obj.m_data) {
        if (hitPtr) {
            m_data.push_back(std::make_unique<DDASRootHit>(*hitPtr));
        } else {
            m_data.push_back(nullptr);
        }
    }
}

/**
 * @brief
 * Performs a deep copy of the data belonging to obj. This assignment operator 
 * is simple at the expense of some safety. The entirety of the data vector is 
 * deleted prior to assignment. If the initialization of the DDASRootHit 
 * objects threw an exception or caused something else to happen that is bad, 
 * then it would be a big problem. The possibility does exist for this to 
 * happen. Coding up a safer version is just more complex, harder to 
 * understand, and will be slower.
 */
DDASRootEvent& DDASRootEvent::operator=(const DDASRootEvent& obj)
{
    if (this != &obj) {
        m_data.clear();
        for (const auto& hitPtr : obj.m_data) {
            if (hitPtr) {
                m_data.push_back(std::make_unique<DDASRootHit>(*hitPtr));
            } else {
                m_data.push_back(nullptr);
            }
        }
    }
    return *this;
}

/**
 * @details
 * Deletes the objects stored in m_data and clear the vector.
 */
DDASRootEvent::~DDASRootEvent(){}

/**
 * @details
 * Appends the pointer to the internal, extensible data array. There is no 
 * check that the object pointed to by the argument exists, so that it is 
 * the user's responsibility to implement.
 */
void DDASRootEvent::AddChannelData(std::unique_ptr<DDASRootHit> channel){
    if (channel) {
        m_data.push_back(std::move(channel));
    }
}
void DDASRootEvent::AddChannelData(std::shared_ptr<DDASRootHit> channel){
    if (channel) {
        m_data.push_back(std::make_unique<DDASRootHit>(*channel));
    }
}

void DDASRootEvent::AddChannelData(DDASRootHit* channel) {
    if (!channel) {
        return; // Avoid adding a null pointer
    }
    m_data.push_back(std::make_unique<DDASRootHit>(*channel));
}

/**
 * @details
 * If data exists return timestamp of first element in the array. This should 
 * be the earliest unit of data stored by this object. If no data exists, 
 * returns 0.
 */ 
Double_t DDASRootEvent::GetFirstTime() const
{
    // Check if m_data is empty before accessing front()
    if (m_data.empty()) {
        return 0; // Return 0 if no data exists
    }
    
    return m_data.front()->getTime();
}

/**
 * @details
 * If data exists return timestamp of last element in the array. This should 
 * be the most recent unit of data stored by this object. If no data exists, 
 * returns 0.
 */
Double_t DDASRootEvent::GetLastTime() const
{
    // Check if m_data is empty before accessing back()
    if (m_data.empty()) {
        return 0; // Return 0 if no data exists
    }
    
    return m_data.back()->getTime();
}

/**
 * @details
 * Calculate and return the timestamp difference between the last and first
 * elements of the data vector. If the data vector is empty, returns 0.
 */
Double_t DDASRootEvent::GetTimeWidth() const
{
    return GetLastTime() - GetFirstTime();
}

/**
 * @details
 * Deletes the DDASRootHit data objects and resets the  size of the extensible 
 * data array to zero. 
 */
void DDASRootEvent::Reset(){
    // Clear the array and resize it to zero
    m_data.clear();
}
