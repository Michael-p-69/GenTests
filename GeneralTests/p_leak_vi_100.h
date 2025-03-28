#pragma once
#include <Unison.h>
#include <CoreFamily.h>
#include <map>
#include "enums.h"

namespace REN{
namespace GeneralTests{

/** @file
    @brief Defines @ref ConfigureGenericPins "ConfigureGenericPins" Test Method */
/** @brief %Sets up the generic DxV pins controlled by the ECBITs. Everything done in one block is done in a single EBIT burst. */
class leakVI100 : public LTXC::CoreFamily
{
 
 public:
    leakVI100();
    virtual ~leakVI100() { }
    
    virtual TM_RETURN        Execute();
    virtual BoolM            ParamCheck();
    virtual BoolM            Initialize();
 

protected:


    EnumS<REN::UTILS::FlowSequence> Sequence;   
    FloatS Settle;
    BoolS Store; 
    ArrayOfPinML    DUTPins;   
    ArrayOfFloatS   Forced;   
    ArrayOfObjectS  Limits;
    
    virtual PinM pin_redirect(const PinM& alias) const; 
    
    
    //TODO: need to add the condition or maybe the group for the delta test.
    static std::map<REN::UTILS::FlowSequence, std::map<PinM, FloatM>> _delta_map; 
    
    EnumS<REN::UTILS::FlowSequence> pre_select;
    EnumS<REN::UTILS::FlowSequence> post_select; 
    BoolS ClearMap;
    
    
private:
    PinML _get_aliased(const PinML& pins); 
};

}
}
