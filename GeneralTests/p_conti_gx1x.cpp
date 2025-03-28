#include "p_conti_gx1x.h"
#include <datalog_generate.h>
#include "printers.h"
#include "result_map.h"

namespace REN{
namespace GeneralTests{

    TM_RETURN  contiGX1X::Execute(){
        Profiler P(__PRETTY_FUNCTION__);
        
        PinML static_pins = "";
        PinML forcing_pins = ""; 
        PinML forcing_pins_raw = ""; 
        
        
        PinML cres_forcing_pins = ""; 
        PinML cres_forcing_pins_raw = ""; 
        
        PinML cres_delta_pins = ""; 
        
        TMResultM local_result = TM_PASS; 
        
        for(size_t entry = 0; entry < StaticPins.GetSize(); ++entry){
            for(size_t pin = 0; pin < StaticPins[entry].GetNumPins(); ++pin){
                static_pins += pin_redirect(StaticPins[entry][pin]); 
            }
          
        }
        
        std::vector< std::map<StringS,StringS>> ConditionMaps(DUTPins.GetSize()); 
        std::map<PinM,FloatS> intial_force_map; 
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            for(size_t pin = 0; pin < DUTPins[entry].GetNumPins(); ++pin){
                forcing_pins += pin_redirect(DUTPins[entry][pin]); 
                forcing_pins_raw += DUTPins[entry][pin];
                intial_force_map.insert({DUTPins[entry][pin],Forced[entry]});
            }
            
            ConditionMaps[entry].insert({"$FORCED",Forced[entry].GetText()});
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
        
        std::vector< std::map<StringS,StringS>> ConditionMapsPass2(DUTPinsMeasure2.GetSize()); 
        std::map<PinM,FloatS> second_force_map; 
        for(size_t entry = 0; entry < DUTPinsMeasure2.GetSize(); ++entry){
            for(size_t pin = 0; pin < DUTPinsMeasure2[entry].GetNumPins(); ++pin){
                cres_forcing_pins += pin_redirect(DUTPinsMeasure2[entry][pin]); 
                cres_forcing_pins_raw += DUTPinsMeasure2[entry][pin];
                second_force_map.insert({DUTPins[entry][pin],Forced[entry]});
            }
            
            ConditionMapsPass2[entry].insert({"$FORCED",ForcedMeasure2[entry].GetText()});
            ConditionMapsPass2[entry].insert({"$SEQ",REN::UTILS::PrintFlowSequence(Sequence)});
            
        }
        
        std::map<PinM, std::map<StringS,StringS>> ConditionMapsCRES; 
        for(size_t entry = 0; entry < CRESPins.GetSize(); ++entry){
            for(size_t pin = 0; pin < CRESPins[entry].GetNumPins(); ++pin){ 
                cres_delta_pins += CRESPins[entry][pin];
                auto delta = intial_force_map.at(CRESPins[entry][pin]) - second_force_map.at(CRESPins[entry][pin]);
                ConditionMapsCRES.insert({CRESPins[entry][pin],std::map<StringS,StringS>()});
                ConditionMapsCRES.at(CRESPins[entry][pin]).insert({"$FORCED",delta.GetText()});
                ConditionMapsCRES.at(CRESPins[entry][pin]).insert({"$SEQ",REN::UTILS::PrintFlowSequence(Sequence)});
            } 
        }
        
        
        
        PinML all_pins = static_pins + forcing_pins + cres_forcing_pins;
    
    
        //TODO: Profile and move to setup routine?
        VI.Connect(all_pins, VI_TO_DUT,VI_MODE_LOCAL);
        
        //TODO: make this nicer
        VI.SetClampsI(all_pins,2mA,-2mA);
        VI.SetClampsV(all_pins,1.6V,-1.6V);
        
        VI.SetMeasureVRange(all_pins,5.7V); 
        VI.SetMeasureIRange(all_pins,5mA);
        
        
        for(size_t entry = 0; entry < StaticPins.GetSize(); ++entry){
            VI.Force(_get_aliased(StaticPins[entry]),VI_FORCE_V,StaticForce[entry],StaticForce[entry]*2,1e-3,-1e-3);
        }
        
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            VI.Force(_get_aliased(DUTPins[entry]),VI_FORCE_I,Forced[entry],Forced[entry]*2,3,-1.6);
        }
        VI.Gate(all_pins,VI_GATE_ON);
        TIME.Wait(Settle);
        
        
        VI.SetMeasureSamples(forcing_pins,100);
        FloatM1D ResultV; 
        VI.MeasureVAverage(forcing_pins,ResultV,0mA);
        
        const auto result_map = REN::UTILS::ParseResults(forcing_pins_raw,ResultV);
        if(Store){
            _delta_map.insert({REN::UTILS::FlowSequence(Sequence),result_map});
        }
        
        std::map<PinM,FloatM> cres_pass_map; 
        if(EnableCRES){
            for(size_t entry = 0; entry < DUTPinsMeasure2.GetSize(); ++entry){
                VI.Force(_get_aliased(DUTPinsMeasure2[entry]),VI_FORCE_I,ForcedMeasure2[entry],ForcedMeasure2[entry]*2,3,-3);
            }
        
            TIME.Wait(Settle);
        
            VI.SetMeasureSamples(cres_forcing_pins,100);
            FloatM1D ResultVCRES; 
            VI.MeasureVAverage(cres_forcing_pins,ResultV,0mA);
        
            cres_pass_map = REN::UTILS::ParseResults(cres_forcing_pins_raw,ResultVCRES);
        }
        
        VI.SetMeasureVRange(all_pins,5.7);
        VI.Force(all_pins,VI_FORCE_I,0,300uA,5.0V,-1.5V);   
        VI.Gate(all_pins,VI_GATE_OFF_LOZ);
        VI.Disconnect(all_pins); 
        
        IntM pin_fails = 0; 
        for(size_t entry = 0; entry < DUTPins.GetSize(); ++entry){
            auto limit = LimitStruct(Limits[entry]);
  
            for(size_t pin = 0; pin < DUTPins[entry].GetNumPins(); ++pin){
                const auto temp_result = REN::UTILS::DatalogParametricRenesas(limit,DUTPins[entry][pin],result_map.at(DUTPins[entry][pin]),ConditionMaps[entry]);
                DLOG.AccumulateResults(local_result,temp_result);
                
                pin_fails += temp_result;
            }
        }
        this->SetValue(pin_fails);
        
        if(EnableCRES){
            for(size_t entry = 0; entry < DUTPinsMeasure2.GetSize(); ++entry){
                auto limit = LimitStruct(LimitsMeasure2[entry]);
      
                for(size_t pin = 0; pin <DUTPinsMeasure2[entry].GetNumPins(); ++pin){
                    const auto temp_result = 
                        REN::UTILS::DatalogParametricRenesas(limit,DUTPinsMeasure2[entry][pin],cres_pass_map.at(DUTPinsMeasure2[entry][pin]),ConditionMapsPass2[entry]);
                    DLOG.AccumulateResults(local_result,temp_result);    
                }
            }
            
            for(size_t entry = 0; entry < CRESPins.GetSize(); ++entry){
                auto limit = LimitStruct(CRESLimits[entry]);
      
                for(size_t pin = 0; pin <CRESPins[entry].GetNumPins(); ++pin){
                    FloatM cres = 
                        (result_map.at(CRESPins[entry][pin]) - cres_pass_map.at(CRESPins[entry][pin])) / 
                        (intial_force_map.at(CRESPins[entry][pin]) - second_force_map.at(CRESPins[entry][pin]));
                    const auto temp_result = REN::UTILS::DatalogParametricRenesas(limit,CRESPins[entry][pin],cres,ConditionMapsCRES.at(CRESPins[entry][pin]));
                    DLOG.AccumulateResults(local_result,temp_result);
                }
            }
        
        
        }
    
        SetResult(local_result);
        return TM_RETURN::TM_HAS_VALUE_AND_RESULT;
    }
    
    PinM contiGX1X::pin_redirect(const PinM& alias) const{
        return alias;
    }
    
    PinML contiGX1X::_get_aliased(const PinML& pins){
        PinML aliased = "";
        for(size_t pin = 0; pin < pins.GetNumPins(); ++pin){
            aliased += pin_redirect(pins[pin]); 
        }
        return aliased;
    }


    BoolM contiGX1X::Initialize()
    { 
        Profiler P(__PRETTY_FUNCTION__);

        BoolM initialize_status = true;

       
        return initialize_status;
    }
 
    BoolM contiGX1X::ParamCheck()
    {
        StringS error_message = "";
        BoolM param_check_status = true;
        //return true;
        
//        for(size_t entry =0; entry < StaticForce.GetSize(); ++entry){
//            if(StaticForce[entry].GetUnits() != "V"){
//                param_check_status = false; 
//                error_message += "Static Force Conditions must all be Voltage!\n";
//                ERR.ReportError(ERR_INVALID_PARAMETER,"Static Force Conditions must all be Voltage!\n", StaticForce,entry,NO_SITES,UTL_VOID);
//            }
//        }
//        
//        for(size_t entry =0; entry < Forced.GetSize(); ++entry){
//            if(Forced[entry].GetUnits() != "A"){
//                param_check_status = false; 
//                error_message += "Continutiy Force Conditions must all be Current!\n";
//                ERR.ReportError(ERR_INVALID_PARAMETER,"Continutiy Force Conditions must all be Current!\n", Forced,entry,NO_SITES,UTL_VOID);
//            }
//        }
//        
//         for(size_t entry =0; entry < Limits.GetSize(); ++entry){
//            auto dlog_text= LimitStruct(Limits[entry]).GetDlogText();
////            if(
////                dlog_text.Find("$SEQ")      == -1 ||
////                dlog_text.Find("$FORCED")   == -1 ){
////                
////                param_check_status = false; 
////                error_message += "Datalog Text does not assign the required variables!\n";
////                ERR.ReportError(ERR_INVALID_PARAMETER,"Datalog Text does not assign the required variables!\n", Limits,entry,NO_SITES,UTL_VOID);
////            }
//         }
        
        
        return param_check_status;
    }
    
    contiGX1X::contiGX1X()
    {
        AddInputParameter(Sequence,"Sequence");
        SetParameterAttribute("Sequence","parameter-group","Setup");
        SetParameterAttribute("Sequence","description",
            "<h3>Used to differentiate multiple calls to the same test conditions \
            that differ only in where in the flow they are called.</h3> <br><br> The $SEQ placeholder must be included in the datalog prototype for the limit structure!\
            <ul>\
            <li>This is a <b>REN::UTILS::FlowSequence<b> enumerated type with options of:\
                <ul>\
                    <li><b>PRE</b></li>\
                    <li><b>POST</b></li>\
                    <li><b>DELTA</b></li>\
                </ul>\
                </li>\
            <li>When <b>PRE</b> or <b>POST</b> is called the behavoir is identical apart from the datalog</li>\
            <li>If the Store paramter is <b>TRUE then the results will be stored for a later Delta measure</li>\
            <li>when <b>DELTA</b> is called the test will perform a delta between the previously stored <b>PRE</b> and <b>POST</b> and datalog it vs the limits presented.\
            Calling <b>DELTA</b> will also clear the stored limits. If the <b>PRE</b> and <b>POST</b> values are not present a runtime erorr will occur!</li>\
            </ul>"
        );
        SetParameterAttribute("Sequence","tooltip",
            "Used to differentiate multiple calls to the same test conditions that differ only in where in the flow they are called. \
            The $SEQ placeholder must be included in the datalog prototype for the limit structure!"
        );
        
        Settle.SetUnits("s");
        AddInputParameter(Settle,"Settle", "100us");
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
        
        AddInputParameter(EnableCRES,"EnableCRES","FALSE");
        SetParameterAttribute("EnableCRES","parameter-group","Setup");
        SetParameterAttribute("EnableCRES","display-if","NOT(..Sequence = FlowSequence:DELTA)");
        SetParameterAttribute("EnableCRES","tooltip",
            "Should a second pass at a different current be performed to calculate the 2 point CRES measurement? NOTE: Only the first pass will be STORED if STORE is TRUE for later DELTA Tests! Right click to read more about CRES."
        );
        SetParameterAttribute("EnableCRES","description",
            "<h3>What is CRES?</h3>\
            <p>CRES is the variable contact resistance between the DUT pin and the Tester instrument or connection.\
            This changes over time as the pin degrades or gets dirty. Since it is in series with our instrumentation it\
            can be difficult to measure.</p>\
            <p>In order to measure CRES, first we should understand how continuity works. Continuity uses the ESD structure of the chip,\
            illustrated simply below, to ensure there is a path from that DUT pin back to the tester.</p><br>\
            <img src=\"/home/rdumene/Programs/DxV_Template/Libraries/GeneralTests/Docs/esd.PNG\" alt=\"ESD Structure\"><br>\
            <p>In general this is done by pulling a current out of the DUT pin and observing the negative\
            voltage generated by the ESD diode connected to ground.</p><br>\
            <img src=\"/home/rdumene/Programs/DxV_Template/Libraries/GeneralTests/Docs/conti.PNG\" alt=\"Conti Routine\"><br>\
            <p>As can be seen in the diagram, CRES is present between both the ground connection and the pin connection.\
            There is also the diode voltage to content with. Thankfully the diode voltate does not change much with current.\
            In order to calcualte the CRES we can do a two pass measurement.</p>\
            <p>First we perform continuity at one current and record the voltage, then we perform the measurement again with\
            a different current and record the result. Since we can assume the diode voltage is constant, we can take the delta\
            of the two voltages to determine the effect of the ohmic drop in the path. We can then divide that delta votlage by\
            the delta current to find the effective resistance of the CRES on the chip. This CRES will include both ground and pin paths.</p><br>\
            <img src=\"/home/rdumene/Programs/DxV_Template/Libraries/GeneralTests/Docs/ohms.PNG\" alt=\"Ohms Law Applied\"><br>\
            <p>In general we care less about the absolute value of the CRES, and more about how it changes over time or between\
            pins or sites. This can allow us to detect a problem beofre causing damage to the test hardware or a test escape.\
            Due to the requirement to look for variation in CRES rather than the asbolute value, the contribution of the ground\
            path can be generally ignored as it will contribute to all CRES measurements simialrly as an offset.</p>"
         );
        
        AddInputParameter(StaticPins,"StaticPins");
        SetParameterAttribute("StaticPins","parameter-group","Static Pin Setup");
        SetParameterAttribute("StaticPins","display-if","NOT(..Sequence = FlowSequence:DELTA)");
        SetParameterAttribute("StaticPins","tooltip",
            "Which pins will be held at a constant voltage during the test? These pins are not measured! Each row is a seperate force condition."
        );
        
        AddInputParameter(StaticForce,"StaticForce");
        SetParameterAttribute("StaticForce","parameter-group","Static Pin Setup");
        SetParameterAttribute("StaticForce","display-if","NOT(..Sequence = FlowSequence:DELTA)");
        SetParameterAttribute("StaticForce","tooltip",
            "The constant voltage applied to the pin list during the test"
        );
        
    
        AddInputParameter(DUTPins,"DUTPins");
        SetParameterAttribute("DUTPins","parameter-group","Simultaneous Continutiy");
        SetParameterAttribute("DUTPins","tooltip",
            "The pins to be tested for continuity simultaneously. Each row is a single pin list/force/limit combination. Multiple rows can be used for multiple limits on different pins with the same conditions!"
        );
        
        AddInputParameter(Forced,"ForcedCurrent");
        SetParameterAttribute("ForcedCurrent","parameter-group","Simultaneous Continutiy" );
        SetParameterAttribute("ForcedCurrent","display-if","NOT(..Sequence = FlowSequence:DELTA)");
        SetParameterAttribute("ForcedCurrent","tooltip",
            "The continutiy current for these pins. The $FORCED placeholder must be included in the datalog prototype for the limit structure!"
        );
        
        AddInputParameter(Limits,"Limits");
        SetParameterAttribute("Limits","parameter-group","Simultaneous Continutiy" );
        SetParameterAttribute("Limits", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("Limits","tooltip",
            "The limit struct that will be used to generate the datalog for the per pin value. Take note to ensure the prototpye string is correct and inclues the $SEQ and $FORCED variables!"
        );
        
        AddInputParameter(DUTPinsMeasure2,"DUTPinsMeasure2");
        SetParameterAttribute("DUTPinsMeasure2", "display-if", "..EnableCRES");
        SetParameterAttribute("DUTPinsMeasure2","parameter-group","Simultaneous CRES Pass");
        SetParameterAttribute("DUTPinsMeasure2","tooltip",
            "The pins to be tested for continuity simultaneously in a second pass. This will be used to calculate CRES with a 2 point calcualion. Each row is a single pin list/force/limit combination. Multiple rows can be used for multiple limits on different pins with the same conditions!"
        );
        
        AddInputParameter(ForcedMeasure2,"ForcedMeasure2");
        SetParameterAttribute("ForcedMeasure2", "display-if", "..EnableCRES");
        SetParameterAttribute("ForcedMeasure2","parameter-group","Simultaneous CRES Pass" );
        SetParameterAttribute("ForcedMeasure2","tooltip",
            "The continutiy current for these pins. This will be used to calculate CRES with a 2 point calcualion. The $FORCED placeholder must be included in the datalog prototype for the limit structure!"
        );
        
        AddInputParameter(LimitsMeasure2,"LimitsMeasure2");
        SetParameterAttribute("LimitsMeasure2", "display-if", "..EnableCRES");
        SetParameterAttribute("LimitsMeasure2","parameter-group","Simultaneous CRES Pass" );
        SetParameterAttribute("LimitsMeasure2", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("LimitsMeasure2","tooltip",
            "The limit struct that will be used to generate the datalog for the per pin value. Take note to ensure the prototpye string is correct and inclues the $SEQ and $FORCED variables!"
        );
        
        AddInputParameter(CRESPins,"CRESPins");
        SetParameterAttribute("CRESPins", "display-if", "..EnableCRES");
        SetParameterAttribute("CRESPins","parameter-group","CRES Logging");
        SetParameterAttribute("CRESPins","tooltip",
            "The pins to calculate CRES on. Each CRES pin must be in both passes of continuity!"
        );
        AddInputParameter(CRESLimits,"CRESLimits");
        SetParameterAttribute("CRESLimits", "display-if", "..EnableCRES");
        SetParameterAttribute("CRESLimits","parameter-group","CRES Logging" );
        SetParameterAttribute("CRESLimits", "restrict-type", "LimitStruct"); 
        SetParameterAttribute("CRESLimits","tooltip",
            "The limit struct that will be used to generate the datalog for the per pin CRES. Take note to ensure the prototpye string is correct and inclues the $SEQ and $FORCED variables!"
        );
        
        this->SetValueType(IntM()); 

    }
    
    std::map<REN::UTILS::FlowSequence, std::map<PinM, FloatM>> contiGX1X::_delta_map; 
    

//#ifdef ENABLE_REN_GENRIC
TMFAMILY_CLASS(REN::GeneralTests::contiGX1X)
//#endif 


}//GeneralTests
}//REN
