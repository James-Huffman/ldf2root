/*****************************************/
/* This channel initializes a new        */
/* DDAS channel event.                   */
/*****************************************/

#include "ddaschannel_legacy.h"

ClassImp(ddaschannel);

ddaschannel::ddaschannel() : TObject() {
  channelnum = 0;
  chanid = 0;
  slotid = 0;
  crateid = 0;
  channelheaderlength = 0;
  channellength = 0;
  finishcode = 0;
  overflowcode = 0;
  tracelength = 0;
  ModMSPS = 0;

  timelow = 0;
  timehigh = 0;
  timecfd = 0;
  cfdtrigsourcebit = 0;
  cfdfailbit = 0;
  energy = 0;
  
  time = 0.;
  cfd = 0.;
  
  energySums.reserve(4);
  qdcSums.reserve(8);
  trace.reserve(200);
}

ddaschannel::ddaschannel(const ddaschannel& obj): 
  TObject(obj), 
  time(obj.time),
  coarsetime(obj.coarsetime),
  cfd(obj.cfd),
  energy(obj.energy),
  timehigh(obj.timehigh),
  timelow(obj.timelow),
  timecfd(obj.timecfd),
  channelnum(obj.channelnum),
  finishcode(obj.finishcode),
  channellength(obj.channellength),
  channelheaderlength(obj.channelheaderlength),
  overflowcode(obj.overflowcode),
  chanid(obj.chanid),
  slotid(obj.slotid),
  crateid(obj.crateid),
  id(obj.id),
  cfdtrigsourcebit(obj.cfdtrigsourcebit),
  cfdfailbit(obj.cfdfailbit),
  tracelength(obj.tracelength),
  ModMSPS(obj.ModMSPS),
  energySums(obj.energySums),
  qdcSums(obj.qdcSums),
  trace(obj.trace)
{}

ddaschannel& ddaschannel::operator=(const ddaschannel& obj) {
  if (this!=&obj) {
    channelnum = obj.channelnum;
    chanid = obj.chanid;
    slotid = obj.slotid;
    crateid = obj.crateid;
    channelheaderlength = obj.channelheaderlength;
    channellength = obj.channellength;
    finishcode = obj.finishcode;
    overflowcode = obj.overflowcode;
    tracelength = obj.tracelength;
    ModMSPS = obj.ModMSPS;
    timelow = obj.timelow;
    timehigh = obj.timehigh;
    timecfd = obj.timecfd;
    cfdtrigsourcebit = obj.cfdtrigsourcebit;
    cfdfailbit = obj.cfdfailbit;
    energy = obj.energy;
    time = obj.time;
    cfd = obj.cfd;
    energySums = obj.energySums;
    qdcSums = obj.qdcSums;
    trace = obj.trace;
  }
  return *this;
}

void ddaschannel::Reset() {
  channelnum = 0;
  chanid = 0;
  slotid = 0;
  crateid = 0;
  channelheaderlength = 0;
  channellength = 0;
  finishcode = 0;
  tracelength = 0;
  ModMSPS = 0;
  timelow = 0;
  timehigh = 0;
  timecfd = 0;
  energy = 0;
  cfdtrigsourcebit = 0;
  cfdfailbit = 0;
  time = 0;
  cfd = 0;
  
  energySums.clear();
  qdcSums.clear();
  trace.clear();
}

ddaschannel::~ddaschannel() {}

void ddaschannel::SetChannelID(UInt_t data) {
  chanid = ((Int_t) ((data & CHANNELIDMASK)));
}

void ddaschannel::SetSlotID(UInt_t data) {
  slotid = ((Int_t) ((data & SLOTIDMASK) >> 4));
}

void ddaschannel::SetCrateID(UInt_t data) {
  crateid = ((Int_t) ((data & CRATEIDMASK) >> 8));
}

void ddaschannel::SetChannelHeaderLength(UInt_t data) {
  channelheaderlength = ((Int_t) ((data & HEADERLENGTHMASK) >> 12 ));
}

void ddaschannel::SetChannelLength(UInt_t data) {
  channellength = ((Int_t) ((data & CHANNELLENGTHMASK) >> 17));
}

void ddaschannel::SetOverflowCode(UInt_t data) {
  overflowcode = ((Int_t) ((data & OVERFLOWMASK) >> 30));
}

void ddaschannel::SetFinishCode(UInt_t data) {
  finishcode = ((Int_t) ((data & FINISHCODEMASK) >> 31 ));
}

void ddaschannel::SetID(UInt_t data) {
  //broken function
}

void ddaschannel::SetTimeLow(UInt_t data) {
  timelow = data;
}

void ddaschannel::SetTimeHigh(UInt_t data) {
  timehigh = ((Int_t) ((data & LOWER16BITMASK)));
}

void ddaschannel::SetTimeCFD(UInt_t data) {
  //check on the module MSPS and pick the correct cfd unpacking algorithm 
  if(ModMSPS == 100) {
    // 100 MSPS modules don't have trigger source bits
    cfdtrigsourcebit = 0;
    // cfd fail bit in bit 31
    cfdfailbit = ((Int_t) ((data & BIT31MASK) >> 31 ));  //modified CP;   
    // remainder in the upper 16 bits of the word
    timecfd = ((Int_t) ((data & BIT30to16MASK) >> 16));
  } else if (ModMSPS == 250) {
    // cfd fail bit in bit 31
    cfdfailbit = ((Int_t) ((data & BIT31MASK) >> 31 ));  //modified CP;     
    cfdtrigsourcebit = ((Int_t) ((data & BIT30MASK) >> 30 ));  //modified CP;    
    timecfd = ((Int_t) ((data & BIT29to16MASK) >> 16));
  } else if (ModMSPS == 500) {
    // no fail bit in 500 MSPS modules
    cfdfailbit = 0;      
    cfdtrigsourcebit = ((Int_t) ((data & BIT31to29MASK) >> 29 ));
    timecfd = ((Int_t) ((data & BIT28to16MASK) >> 16));
  }
}

void ddaschannel::SetTime() {
  //check on the module MSPS and pick the correct time algorithm 
  if(ModMSPS == 100) {
    time = (timecfd/32768. + timelow + timehigh * 4294967296.)*10.0;
  } else if (ModMSPS == 250) {
    time = 8.0*(timelow + timehigh * 4294967296.0) + (timecfd/16384.0 - cfdtrigsourcebit) * 4.0;
  } else if (ModMSPS == 500) {
    time =  10.0*(timelow + timehigh * 4294967296.0) + (timecfd/8192.0 + cfdtrigsourcebit - 1 ) * 2.0;
  }
}

void ddaschannel::SetCoarseTime() {
  //check on the module MSPS and pick the correct time algorithm 
  if(ModMSPS == 100) {
    coarsetime = (timelow + timehigh * 4294967296.)*10.0;
  } else if (ModMSPS == 250) {
    coarsetime =  8.0*(timelow + timehigh * 4294967296.0);
  } else if (ModMSPS == 500) {
    coarsetime =  10.0*(timelow + timehigh * 4294967296.0);
  }
}

void ddaschannel::SetEnergy(UInt_t data) {
  energy = ((Int_t) ((data & LOWER16BITMASK)));
}

void ddaschannel::SetTraceLength(UInt_t data) {
  tracelength = ((Int_t) ((data & UPPER16BITMASK) >> 16));
}

void ddaschannel::SetModMSPS(UInt_t data) {
  ModMSPS = ((Int_t) (data & LOWER16BITMASK) );
}

void ddaschannel::SetEnergySums(UInt_t data) {
  energySums.push_back(data);
}

void ddaschannel::SetQDCSums(UInt_t data) {
  qdcSums.push_back(data);
}

void ddaschannel::SetTraceValues(UInt_t data) {
  //cout << "set trace values " << data << endl;
  trace.push_back((data & LOWER16BITMASK));
  trace.push_back((data & UPPER16BITMASK)>>16);
  //cout << "set trace values1 " << data << endl;
}

void ddaschannel::UnpackChannelData(const uint32_t *data) {

  //put all variables in known state before unpacking
  Reset();

  //first four words of data identifiers, times, and energies
  //cout << "data ----------" << endl;
  //cout << data[0] << " " << data[1] << " " << data[2] <<" " << data[3] << " " << data[4] <<endl;

  //data[0] is the number of half words in the event, skip it
  // if you need to use the data here, please read the dereference (i.e *data++ instead) - JH
  data++;

  //next word is the module revision, adc bit, and msps
  SetModMSPS(*data++);

  //Using the first word of DDAS information extract channel identifiers
  SetChannelID(*data);
  SetSlotID(*data);
  SetCrateID(*data);
  SetChannelHeaderLength(*data);
  SetChannelLength(*data);
  SetOverflowCode(*data);
  SetFinishCode(*data++);

  // set most significant bits of time stamp
  SetTimeLow(*data++);

  // set least significant bits of time stamp
  SetTimeHigh(*data);

  //set cfd time
  SetTimeCFD(*data++);

  //set the full time
  SetTime();

  //set energy
  SetEnergy(*data);

  //set trace length
  SetTraceLength(*data++);

  // finished upacking the minimum set of data

  //determine if more data should be unpacked based on a comparison between channel header length
  //and channel length
  if(channelheaderlength != channellength) {
    //cout << "extra info unpack " <<endl;
    //more unpacking data
    if(channellength != (channelheaderlength + tracelength/2)) {
      std::cout << "Inconsistent lengths between channel length - " << channellength
           << " , header length - " << channelheaderlength 
           << " , and trace length - " << tracelength << std::endl;
           std::cout << "This is a fatal error - data is corrupted, please investigate " << std::endl;
      exit(1);
    }

    //if channel header length is 8 then the extra 4 words are energy sums and baselines
    if(channelheaderlength == 8) {
      for(int z=0; z<4; z++) {
        SetEnergySums(*data++);
      }
    }
    //if channel header length is 12 then the extra 8 words are QDC sums
    if(channelheaderlength == 12) {
      for(int z=0; z<8; z++) {
        SetQDCSums(*data++);
      }
    }

    //if channel header length is 16 then the extra 12 words are energy and QDC sums
    if(channelheaderlength == 16) {
      for(int z=0; z<4; z++) {
        SetEnergySums(*data++);
      }
      for(int z=0; z<8; z++) {
        SetQDCSums(*data++);
      }
    }

    // if trace length is non zero, retrieve the trace
    if(tracelength !=0) {
      //cout << "trace unpack "<<endl;
      for(Int_t z = 0; z < tracelength/2; z++) {
        //upack two 12 bit trace values from 32 bit data word
        SetTraceValues(*data++);
      }   
    } //finished unpacking trace

  } //finished unpacking extra data
  
}
