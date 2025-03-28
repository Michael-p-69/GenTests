#include "e_log_delta.h"

namespace REN{
namespace GeneralTests{

    TM_RETURN  logDelta::Execute(){
        Profiler P(__PRETTY_FUNCTION__);
        TMResultM local_result; 
        
        //set ranges, run ramp on VI100 or PMVI, capture oputput, switch based on VI100, PMVI or Gx1x on output, prefer Gx1 or Gx1 Attenuated TMU
        
        
        
     
        SetResult(local_result);
        return TM_RETURN::TM_HAS_VALUE_AND_RESULT;
    }



    BoolM logDelta::Initialize()
    { 
        Profiler P(__PRETTY_FUNCTION__);

        BoolM initialize_status = true;

       
        return initialize_status;
    }
 
    BoolM logDelta::ParamCheck()
    {
        StringS error_message;
        BoolM param_check_status = true;
        
        return param_check_status;
    }
    
    //TODO: figure out how to fill from expression from other blocks return value 
    logDelta::logDelta()
    {   
        AddInputParameter(LHS,"LHS");
        SetParameterAttribute("LHS","parameter-group","Deltas");
        SetParameterAttribute("InputPins","tooltip",
            "Left Hand Side of delta expression. Result = LHS - RHS!"
        );
        
        AddInputParameter(RHS,"RHS");
        SetParameterAttribute("RHS","parameter-group","Deltas");
        SetParameterAttribute("RHS","tooltip",
            "Right Hand Side of delta expression. Result = LHS - RHS!"
        );
        
        AddInputParameter(Limits,"Limits");
        SetParameterAttribute("Limits","parameter-group","Deltas");
        SetParameterAttribute("Limits", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("Limits","tooltip",
            "Limit for each delta."
        );
        
    }
    
//#ifdef ENABLE_REN_GENRIC
TMFAMILY_CLASS(REN::GeneralTests::logDelta)
//#endif 



}
}
