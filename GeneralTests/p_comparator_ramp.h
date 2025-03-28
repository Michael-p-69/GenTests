#pragma once
#include <Unison.h>
#include <CoreFamily.h>
#include <map>
#include "enums.h"

namespace REN{
namespace GeneralTests{

enum class VI100_RAMP_TYPES{  
    GATED,
    TRIGGERED  
};

class compRampVi100 : public LTXC::CoreFamily
{
 
 public:
    compRampVi100();
    virtual ~compRampVi100() { }
    
    TM_RETURN        Execute();
    BoolM            ParamCheck();
    BoolM            Initialize();
 

protected:

    EnumS<VI100_RAMP_TYPES> ramp_type;
    BoolS monitor_in_delay;
    BoolS capture_th_delay;
    BoolS capture_out_delay;
    StringS sync_ref; 
    
    BoolS perform_connect;
    BoolS perform_disconnect;
    
    
    StringS digital_pattern;
    UnsignedS digitze_samples;
    UnsignedS timeout_samples; 
    FloatS max_sample_rate; 
    FloatS fail_period; 
    UnsignedS hard_oversample; 

    PinM input_pin; 
    FloatS start;
    FloatS stop; 
    UnsignedS force_steps; 
    FloatS force_bandwidth; 
    
    PinM input_pin_sense;
    BoolS set_vih_in;
    FloatS vih_in; 
    //output param for trheshold
    
    
    PinM output_pin_th;
    BoolS set_vih_th;
    FloatS vih_th; 
    BoolS log_threshold_delay;
    
    PinM output_pin_delay;
    BoolS set_vih_delay;
    FloatS vih_delay; 
    BoolS log_output_delay;
    
    FloatM threshold; 
    LimitStruct ThresholdLimit;
    FloatM thDelay; 
    LimitStruct thDelayLimit;
    FloatM OutDelay; 
    LimitStruct OutDelayLimit;
    
    virtual void set_vih(const PinM& pin, const FloatS& vih); 
    virtual void set_vih(const PinML& pin, const FloatS1D& vih); 
    
    virtual PinM pin_redirect_analog(const PinM& alias) const;  
    virtual PinM pin_redirect_digital(const PinM& alias) const; 
    
private:
  
};

}
}
