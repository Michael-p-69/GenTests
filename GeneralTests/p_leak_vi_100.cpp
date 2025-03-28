#include "p_leak_vi_100.h"
#include <datalog_generate.h>
#include "printers.h"
#include "result_map.h"

namespace REN{
namespace GeneralTests{


    TM_RETURN  leakVI100::Execute(){
        Profiler P(__PRETTY_FUNCTION__);
        
        PinML all_pins = ""; 
        PinML all_pins_raw = ""; 
        
        TMResultM local_result; 
        
        std::vector< std::map<StringS,StringS>> ConditionMaps(DUTPins.GetSize()); 
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            for(size_t pin = 0; pin < DUTPins[entry].GetNumPins(); ++pin){
                all_pins += pin_redirect(DUTPins[entry][pin]);
                all_pins_raw += DUTPins[entry][pin];
            }
            
            ConditionMaps[entry].insert({"$FORCED",REN::UTILS::PrintVoltage(Forced[entry])});
            ConditionMaps[entry].insert({"$SEQ",REN::UTILS::PrintFlowSequence(Sequence)});
            
        }
        
        if(Sequence == REN::UTILS::FlowSequence::DELTA){
            for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
                auto limit = LimitStruct(Limits[entry]);
                for(size_t pin = 0; pin < DUTPins[entry].GetNumPins(); ++pin){
                
                    const FloatM delta = 
                        _delta_map.at(pre_select).at(DUTPins[entry][pin]) -
                        _delta_map.at(post_select).at(DUTPins[entry][pin]);
                    const auto temp_result = REN::UTILS::DatalogParametricRenesas(limit,DUTPins[entry][pin],delta,ConditionMaps[entry]);
                    DLOG.AccumulateResults(local_result,temp_result);
                    
                    
                }
            }
            if(ClearMap){
                _delta_map.clear(); 
            }
            SetResult(local_result);                 
            return TM_RETURN::TM_HAS_RESULT;
        }
        
        
        VI.Connect(all_pins, VI_TO_DUT,VI_MODE_REMOTE);
        
        //TODO: make this nicer
        VI.SetClampsI(all_pins,2mA,-2mA);
        VI.SetClampsV(all_pins,3V,-3V);
        
        VI.SetMeasureVRange(all_pins,5.7);
        VI.SetBandwidth(all_pins,150e3); 
        
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            VI.Force(_get_aliased(DUTPins[entry]),VI_FORCE_V,Forced[entry],Forced[entry]*2,1e-3,-1e-3);
        }
        VI.Gate(all_pins,VI_GATE_ON);
        TIME.Wait(Settle);
        
        
        VI.SetMeasureSamples(all_pins,100);
        FloatM1D ResultI; 
        VI.MeasureIAverage(all_pins,ResultI,0mA);
        
        const auto result_map = REN::UTILS::ParseResults(all_pins_raw,ResultI);
        if(Store){
            _delta_map.insert({REN::UTILS::FlowSequence(Sequence),result_map});
        }
        
        VI.SetMeasureVRange(all_pins,5.7);
        VI.Force(all_pins,VI_FORCE_I,0,300uA,5.0V,-1.5V);   
        VI.Gate(all_pins,VI_GATE_OFF_LOZ);
        VI.Disconnect(all_pins); 
        
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            auto limit = LimitStruct(Limits[entry]);
  
            for(size_t pin = 0; pin < DUTPins[entry].GetNumPins(); ++pin){
                const auto temp_result = 
                    REN::UTILS::DatalogParametricRenesas(limit,DUTPins[entry][pin],result_map.at(DUTPins[entry][pin]),ConditionMaps[entry]);
                DLOG.AccumulateResults(local_result,temp_result);
            }
        }
      
        SetResult(local_result);
        return TM_RETURN::TM_HAS_RESULT;
    }
    
    PinM leakVI100::pin_redirect(const PinM& alias) const{
        return alias;
    }
    
    PinML leakVI100::_get_aliased(const PinML& pins){
        PinML aliased = "";
        for(size_t pin = 0; pin < pins.GetNumPins(); ++pin){
            aliased += pin_redirect(pins[pin]); 
        }
        return aliased;
    }


    BoolM leakVI100::Initialize()
    { 
        Profiler P(__PRETTY_FUNCTION__);

        BoolM initialize_status = true;

       
        return initialize_status;
    }
 
    BoolM leakVI100::ParamCheck()
    {
        StringS error_message;
        BoolM param_check_status = true;

//         for(size_t entry = 0; entry < Forced.GetSize(); ++entry){
//            if(Forced[entry].GetUnits() != "V"){
//                param_check_status = false;
//                ERR.ReportError(ERR_INVALID_PARAMETER,"FORCED value must be voltage for leakage testing!", Forced,entry,NO_SITES,UTL_VOID);
//                
//            }
//         }
//         
//         for(size_t entry =0; entry < Limits.GetSize(); ++entry){
//            auto dlog_text= LimitStruct(Limits[entry]).GetDlogText();
////            if(
////                dlog_text.Find("$SEQ")      == -1 ||
////                dlog_text.Find("$FORCED")   == -1 ){
////                
////                param_check_status = false; 
////                ERR.ReportError(ERR_INVALID_PARAMETER,"Datalog Text does not assign the required variables!\n", Limits,entry,NO_SITES,UTL_VOID);
////            }
//         }
         
         
        
        return param_check_status;
    }
    
    leakVI100::leakVI100()
    {
        AddInputParameter(Sequence,"Sequence");
        SetParameterAttribute("Sequence","parameter-group","Setup");
        SetParameterAttribute("Sequence","description",
            "<h3>Used to differentiate multiple calls to the same test conditions \
            that differ only in where in the flow they are called.</h3> <br><br>\
            <ul>\
            <li>This is a <b>REN::UTILS::FlowSequence<b> enumerated type with options of:\
                <ul>\
                    <li><b>PRE(0-3)</b></li>\
                    <li><b>POST(0-3)</b></li>\
                    <li><b>DELTA</b></li>\
                </ul>\
                </li>\
            <li>When <b>PRE</b> or <b>POST</b> is called the behavoir is identical apart from the datalog</li>\
            <li>The PRE and POST suffixes allow storing multiple data sets if desired for Deltas but thes print as PRE or POST only!</li>\
            <li>If the Store paramter is <b>TRUE then the results will be stored for a later Delta measure</li>\
            <li>when <b>DELTA</b> is called the test will perform a delta between the previously stored <b>PRE</b> and <b>POST</b> and datalog it vs the limits presented which is user selectable. \
            Calling <b>DELTA</b> with the Clear Map Boolean will also clear the stored results. If the <b>PRE</b> and <b>POST</b> values are not present a runtime erorr will occur!</li>\
            </ul>"
        );
        SetParameterAttribute("Sequence","tooltip",
            "Used to differentiate multiple  calls to the same test conditions that differ only in where in the flow they are called. When STROE is enabled this label also labels the Dataset for future use"
        );
        
        Settle.SetUnits("s");
        AddInputParameter(Settle,"Settle","2ms");
        SetParameterAttribute("Settle","parameter-group","Setup");
        SetParameterAttribute("Settle","tooltip",
            "Wait time between GATE_ON and measure"
        );
        
        AddInputParameter(Store,"Store","TRUE");
        SetParameterAttribute("Store","parameter-group","Setup");
        SetParameterAttribute("Store","tooltip",
            "Will the results be stored for later use in the delta measurement? This will over-ride existing stored results for the Sequence!"
        );
        
        AddInputParameter(pre_select,"Pre_Select","FlowSequence:PRE");
        SetParameterAttribute("Pre_Select","parameter-group","Setup");
        SetParameterAttribute("Pre_Select","display-if","..Sequence = FlowSequence:DELTA");
        SetParameterAttribute("Pre_Select","tooltip",
            "The Dataset to act as the 'PRE' value to be used in the delta calculation.  This may not be DETLA!"
        );
        
        AddInputParameter(post_select,"Post_Select","FlowSequence:POST");
        SetParameterAttribute("Post_Select","parameter-group","Setup");
        SetParameterAttribute("Post_Select","display-if","..Sequence = FlowSequence:DELTA");
        SetParameterAttribute("Post_Select","tooltip",
            "The Dataset to act as the 'POST' value to be used in the delta calculation. This may not be DETLA!"
        );
        
        AddInputParameter(ClearMap,"ClearMap","FALSE");
        SetParameterAttribute("ClearMap","parameter-group","Setup");
        SetParameterAttribute("ClearMap","display-if","..Sequence = FlowSequence:DELTA");
        SetParameterAttribute("ClearMap","tooltip",
            "Should the Delta Map be cleared?"
        );
    
        AddInputParameter(DUTPins,"DUTPins");
        SetParameterAttribute("DUTPins","parameter-group","Simultaneous Leakage");
        SetParameterAttribute("DUTPins","tooltip",
            "The pins to be tested for leakage simultaneously. Each row is a single pin list/force value/limit combination. Multiple rows can be used for multiple limits on different pins with the same conditions!"
        );
        
        AddInputParameter(Forced,"ForcedVoltage");
        SetParameterAttribute("ForcedVoltage","parameter-group","Simultaneous Leakage" );
        //SetParameterAttribute("ForcedVoltage","display-if","NOT(..Sequence = FlowSequence:DELTA)");
        SetParameterAttribute("ForcedVoltage","tooltip",
            "The leakage voltage for these pins. The $FORCED placeholder must be included in the datalog prototype for the limit structure!"
        );
        
        
        AddInputParameter(Limits,"Limits");
        SetParameterAttribute("Limits","parameter-group","Simultaneous Leakage" );
        SetParameterAttribute("Limits", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("Limits","tooltip",
            "The limit struct that will be used to generate the datalog for the per pin value. Take note to ensure the prototpye string is correct and inclues the $SEQ and $FORCED variables!"
        );
    }
    
    std::map<REN::UTILS::FlowSequence, std::map<PinM, FloatM>> leakVI100::_delta_map; 
    



//#ifdef ENABLE_REN_GENRIC
TMFAMILY_CLASS(REN::GeneralTests::leakVI100)
//#endif 


}//GeneralTests
}//REN
