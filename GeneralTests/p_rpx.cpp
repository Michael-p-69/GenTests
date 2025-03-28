#include "p_rpx.h"
#include <datalog_generate.h>
#include "printers.h"
#include "result_map.h"

namespace REN{
namespace GeneralTests{

    TM_RETURN  p_rpx::Execute(){
        Profiler P(__PRETTY_FUNCTION__);
       
        PinML all_pins = ""; 
        PinML all_pins_raw = ""; 

        
        //setup all pins
        std::vector<std::map<StringS,StringS>> cond_maps(DUTPins.GetSize());
        std::vector<PinML> redirect_pins(DUTPins.GetSize());
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){  
            all_pins_raw += DUTPins[entry];
            redirect_pins[entry] = _get_aliased(DUTPins[entry]); 
            all_pins += redirect_pins[entry];
            
            cond_maps[entry].insert({StringS("$FORCED"),REN::UTILS::PrintCurrent(ForcedI[entry])}); 
        }
        
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){  
            VI.Force(redirect_pins[entry],VI_FORCE_I,ForcedI[entry],ForcedI[entry]*1.25,Clamp[entry],0V);
            VI.SetMeasureVRange(redirect_pins[entry],Clamp[entry]); 
        }
        if(perform_connect){
            VI.Connect(all_pins,VI_TO_DUT,VI_MODE_LOCAL);      
        }
        VI.Gate(all_pins,VI_GATE_ON); 
        
        VI.SetMeasureSamples(all_pins,Samples); 
        TIME.Wait(Settle); 
        
        FloatM1D MeasureV;
        VI.MeasureVAverage(all_pins,MeasureV,0);
        const auto result_map = REN::UTILS::ParseResults(all_pins_raw,MeasureV); 
        
        VI.Gate(all_pins,VI_GATE_OFF_LOZ); 
        if(perform_disconnect){ 
            VI.Disconnect(all_pins); 
        }
        
        TMResultM local_result = TM_PASS;
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            auto limitV = LimitStruct(LimitsV[entry]);
            auto limitR = LimitStruct(LimitsR[entry]);
  
            for(size_t pin = 0; pin <DUTPins[entry].GetNumPins(); ++pin){
                DLOG.AccumulateResults(local_result, REN::UTILS::DatalogParametricRenesas(limitV,DUTPins[entry][pin],result_map.at(DUTPins[entry][pin]),cond_maps[entry]));
                
                if(PullTypes[entry] == REN::GeneralTests::PULL_TYPE::PULL_DOWN){
                    const auto resistnace = (result_map.at(DUTPins[entry][pin])) / ForcedI[entry];
                    DLOG.AccumulateResults(local_result, REN::UTILS::DatalogParametricRenesas(limitR,DUTPins[entry][pin],
                        resistnace,cond_maps[entry]));
                }
                else{
                    const auto resistnace = (result_map.at(DUTPins[entry][pin])- Clamp[entry]) / ForcedI[entry];
                    DLOG.AccumulateResults(local_result, REN::UTILS::DatalogParametricRenesas(limitR,DUTPins[entry][pin],
                        resistnace,cond_maps[entry]));
                }
            }
        }
       
        SetResult(local_result);
        return TM_RETURN::TM_HAS_RESULT;
    }
    
    PinM p_rpx::pin_redirect(const PinM& alias) const{
        return alias;
    }
    
    PinML p_rpx::_get_aliased(const PinML& pins){
        PinML aliased = "";
        for(size_t pin = 0; pin < pins.GetNumPins(); ++pin){
            aliased += pin_redirect(pins[pin]); 
        }
        return aliased;
    }


    BoolM p_rpx::Initialize()
    { 
        Profiler P(__PRETTY_FUNCTION__);

        BoolM initialize_status = true;

       
        return initialize_status;
    }
 
    BoolM p_rpx::ParamCheck()
    {
        StringS error_message = "";
        BoolM param_check_status = true;
        
        
        return param_check_status;
    }
    
    p_rpx::p_rpx()
    {
        Settle.SetUnits("s");
        AddInputParameter(Settle,"Settle","100us");
        SetParameterAttribute("Settle","parameter-group","Setup");
        SetParameterAttribute("Settle","tooltip",
            "Wait between GATE_ON and measure"
        );
        
        AddInputParameter(Samples,"Samples","20");
        SetParameterAttribute("Samples","parameter-group","Setup");
        SetParameterAttribute("Samples","tooltip",
            "Number of samples to average"
        );
        
        AddInputParameter(perform_connect,"perform_connect","FALSE");
        SetParameterAttribute("perform_connect","parameter-group","Setup");
        SetParameterAttribute("perform_connect","tooltip",
            "Should instrument connect and gate mode be set before testing?"
        );
        AddInputParameter(perform_disconnect,"perform_disconnect","FALSE");
        SetParameterAttribute("perform_disconnect","parameter-group","Setup");
        SetParameterAttribute("perform_disconnect","tooltip",
            "Should instrument disconnect and gate mode be set before testing?"
        );
    
        AddInputParameter(DUTPins,"DUTPins");
        SetParameterAttribute("DUTPins","parameter-group","Simultaneous Measure");
        SetParameterAttribute("DUTPins","tooltip",
            "Pins that will be measured"
        );
        
        AddInputParameter(PullTypes,"PullTypes");
        SetParameterAttribute("PullTypes","parameter-group","Simultaneous Measure");
        SetParameterAttribute("PullTypes","tooltip",
            "Pull Up or Down. If Pull-Up will be calculated to the clamp (supply), if pull down will be calculated to ground. "
        );
        
        
        AddInputParameter(ForcedI,"ForcedCurrent");
        SetParameterAttribute("ForcedCurrent","parameter-group","Simultaneous Measure");
        SetParameterAttribute("ForcedCurrent","tooltip",
            "Current to apply to the pins. Positive current is INTO the DUT."
        );
        
        AddInputParameter(Clamp,"Clamp");
        SetParameterAttribute("Clamp","parameter-group","Simultaneous Measure");
        SetParameterAttribute("Clamp","tooltip",
            "The voltage clamp that should be applied. This must be equal to the supply voltage of any pullups!"
        );
        
        AddInputParameter(LimitsV,"LimitsV");
        SetParameterAttribute("LimitsV","parameter-group","Simultaneous Measure" );
        SetParameterAttribute("LimitsV", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("LimitsV","tooltip",
            "Limits for the raw voltage measure. Ensure the unit type is V and the $FORCED variable is present in the string."
        );
        
        AddInputParameter(LimitsR,"LimitsR");
        SetParameterAttribute("LimitsR","parameter-group","Simultaneous Measure" );
        SetParameterAttribute("LimitsR", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("LimitsR","tooltip",
            "Limits for the calculated resistance. Ensure the unit type is V and the $FORCED variable is present in the string."
        );
        

    }
    

    

//#ifdef ENABLE_REN_GENRIC
TMFAMILY_CLASS(REN::GeneralTests::p_rpx)
//#endif 


}//GeneralTests
}//REN
