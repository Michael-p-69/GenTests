#pragma once
#include <Unison.h>
#include <CoreFamily.h>
#include "enums.h"


//I need to make the aliased versions for the DxV
namespace REN{
namespace GeneralTests{

/** @file
    @brief Defines @ref ConfigureGenericPins "ConfigureGenericPins" Test Method */
/** @brief %Sets up the generic DxV pins controlled by the ECBITs. Everything done in one block is done in a single EBIT burst. */
class contiVI100 : public LTXC::CoreFamily
{
 
 public:
    contiVI100();
    virtual ~contiVI100() { }
    
    TM_RETURN        Execute();
    BoolM            ParamCheck();
    BoolM            Initialize();
 

protected:


    EnumS<REN::UTILS::FlowSequence> Sequence;   
    FloatS Settle;
    BoolS Store; 
    BoolS EnableCRES; 
    
    ArrayOfPinML StaticPins; 
    ArrayOfFloatS   StaticForce;  
    
    ArrayOfPinML    DUTPins;   
    ArrayOfFloatS   Forced;   
    ArrayOfObjectS  Limits;
    
    ArrayOfPinML    DUTPinsMeasure2;   
    ArrayOfFloatS   ForcedMeasure2;   
    ArrayOfObjectS  LimitsMeasure2;
    
    ArrayOfPinML    CRESPins;  
    ArrayOfObjectS  CRESLimits;
    
    virtual PinM pin_redirect(const PinM& alias) const; 
    
    static std::map<REN::UTILS::FlowSequence, std::map<PinM, FloatM>> _delta_map; 
    
    EnumS<REN::UTILS::FlowSequence> pre_select;
    EnumS<REN::UTILS::FlowSequence> post_select; 
    BoolS ClearMap;
    
private:

    PinML _get_aliased(const PinML& pins); 
    
};

}
}
