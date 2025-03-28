#pragma once
#include <Unison.h>
#include <CoreFamily.h>
#include "enums.h"


//I need to make the aliased versions for the DxV
namespace REN{
namespace GeneralTests{

enum class PULL_TYPE{
    PULL_DOWN,
    PULL_UP
};

/** @file
    @brief Defines @ref ConfigureGenericPins "ConfigureGenericPins" Test Method */
/** @brief %Sets up the generic DxV pins controlled by the ECBITs. Everything done in one block is done in a single EBIT burst. */
class p_rpx : public LTXC::CoreFamily
{
 
 public:
    p_rpx();
    virtual ~p_rpx() { }
    
    TM_RETURN        Execute();
    BoolM            ParamCheck();
    BoolM            Initialize();
 

protected:

    
    FloatS Settle;
    UnsignedS Samples;
    BoolS perform_connect;
    BoolS perform_disconnect;
    
    
    ArrayOfPinML                DUTPins;   
    ArrayOfEnumS<PULL_TYPE>     PullTypes; 
    ArrayOfFloatS               ForcedI;
    ArrayOfFloatS               Clamp;  
    
    ArrayOfObjectS              LimitsV; 
    ArrayOfObjectS              LimitsR;
    

    
    virtual PinM pin_redirect(const PinM& alias) const; 
    
    
private:

    PinML _get_aliased(const PinML& pins); 
    
};

}
}
