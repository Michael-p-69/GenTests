#include "p_comparator_ramp.h"

namespace REN{
namespace GeneralTests{

    TM_RETURN  compRampVi100::Execute(){
        Profiler P(__PRETTY_FUNCTION__);

        PinML all_analog = "";
        PinM input_alias = this->pin_redirect_analog(input_pin);
        all_analog += input_alias;
        
        PinML all_digital = "";
        PinM input_digital_alias;
        if(monitor_in_delay) input_digital_alias = this->pin_redirect_digital(input_pin_sense); 
        PinM thresh_digital_alias = this->pin_redirect_digital(output_pin_th); 
        PinM output_digital_alias;
        if(capture_out_delay) output_digital_alias = this->pin_redirect_digital(output_pin_delay); 
        
        if(monitor_in_delay)  all_digital += input_digital_alias;
                              all_digital += thresh_digital_alias;
        if(capture_out_delay) all_digital += output_digital_alias;
        
        VI.SetMeasureVRange(input_alias,std::max(start,stop));
        VI.Force(input_alias,VI_FORCE_V,start, std::max(start,stop),15mA); 
        
        if(perform_connect){
            VI.Connect(all_analog,VI_TO_DUT,VI_MODE_REMOTE); 
            DIGITAL.Connect(all_digital,DIGITAL_DCL_TO_DUT); 
            VI.Gate(all_analog,VI_GATE_ON); 
        }          

        VI.SetBandwidth(input_alias,force_bandwidth);
        const FloatS srcRate           = 500kHz;
        const FloatS sample_period     = 1.0/max_sample_rate;
        
        DIGITAL.ConnectSyncRefs     (digital_pattern);
        if(ramp_type == VI100_RAMP_TYPES::GATED){
            VI.ConnectSourceTrigger     (input_alias,VI_TRIGGER_START,sync_ref);
        }
        else {
            VI.ConnectSourceTrigger     (input_alias,VI_TRIGGER_STEP,sync_ref);
        }
        //TODO: move ramp generation to the initialize function, or maybe parameter check?
        VI.DefineRampWaveform("VrampWf", input_alias, start, stop,force_steps,VI_FORCE_V);
        VI.SetWaveformSampleRate("VrampWf", srcRate); 
        VI.SetWaveformLoopCount("VrampWf", 1);
        VI.SetWaveformRange("VrampWf", std::max(start,stop));
        VI.LoadWaveform("VrampWf");
        VI.SelectWaveform("VrampWf", input_alias);
        VI.PrepareWaveform();
        
        VI.ConnectMeasureTrigger    (input_alias,VI_TRIGGER_STEP,"TrigPat"); 
        VI.ClearMeasurements(input_alias); 
        VI.StartMeasure(input_alias,VI_MEASURE_V,VI_MEASURE_SAMPLES,std::max(start,stop),digitze_samples,1us);
        VI.StartWaveform(input_alias);
        

        
        //TODO: SET VIH for the digital pins if requested!
        PinML pin_set_vih(""); 
        FloatS1D to_set_vih(3);
        size_t idx = 0;
        if(set_vih_in && monitor_in_delay){
            pin_set_vih += input_digital_alias;
            to_set_vih[idx++] = set_vih_in;
        }
        if(set_vih_th){
            pin_set_vih += thresh_digital_alias;
            to_set_vih[idx++] = vih_th;
        }
        if(set_vih_delay && capture_out_delay){
            pin_set_vih += output_digital_alias;
            to_set_vih[idx++] = vih_delay;
        }
        
        if(pin_set_vih.GetNumPins() > 0){
            to_set_vih.Resize(pin_set_vih.GetNumPins());
            set_vih(pin_set_vih,to_set_vih);
        }
        
        
        FloatS ratio = (1.0 /  max_sample_rate) / fail_period;
        if(ratio < FloatS(hard_oversample)) ratio = FloatS(hard_oversample) +1.0; 
        UnsignedS sample_repeat = UnsignedS(ratio) - hard_oversample;
        DIGITAL.ModifyLoopCounter       (digital_pattern+".loop_samples",digitze_samples);
        DIGITAL.ModifyRepeatCounter     (digital_pattern+".sample_rpt",sample_repeat);
        DIGITAL.ModifyRepeatCounter     (digital_pattern+".timeout_rpt",timeout_samples);
        
            
        DIGITAL.ExecutePattern          (digital_pattern);

        VI.StopWaveform(input_alias);
        
        UnsignedM input_count, th_count, output_count; 
        if(monitor_in_delay)  DIGITAL.ReadCounter             (input_digital_alias,DIGITAL_FAIL_COUNTER,input_count,0);
                              DIGITAL.ReadCounter             (thresh_digital_alias,DIGITAL_FAIL_COUNTER,th_count,0);
        if(capture_out_delay) DIGITAL.ReadCounter             (output_digital_alias,DIGITAL_FAIL_COUNTER,output_count,0);

        UnsignedM th_adj     = th_count;
        UnsignedM output_adj = output_count; 
        
        if(monitor_in_delay) th_adj = th_count - input_count;
        if(capture_out_delay && monitor_in_delay) output_adj = output_count - input_count;  

        FloatM1D waveform_measure; 
        VI.ReadMeasureSamples(input_alias,waveform_measure,0V); 

        if(ramp_type == VI100_RAMP_TYPES::GATED){
            VI.DisconnectSourceTrigger     (input_alias,VI_TRIGGER_START);
        }
        else {
            VI.DisconnectSourceTrigger     (input_alias,VI_TRIGGER_STEP);
        }
        VI.DisconnectMeasureTrigger (input_alias,    VI_TRIGGER_STEP);
        DIGITAL.DisconnectSyncRefs();
        
        if(perform_disconnect){
            VI.Gate(all_analog,VI_GATE_OFF_LOZ); 
            VI.Disconnect(all_analog); 
            DIGITAL.Disconnect(all_digital,DIGITAL_DCL_TO_DUT); 
        }
        
        //TODO: need to make multi-site
        FloatS oversample_ratio((1.0 /  max_sample_rate) / fail_period);
        threshold = waveform_measure[(th_count[SITE_1])/UnsignedS(oversample_ratio)];
        
        thDelay = th_adj * fail_period;
        OutDelay = output_adj * fail_period;
        
        
        SetValue(threshold); 
        TMResultM local_result = TM_PASS; 
        SetResult(local_result);
        return TM_RETURN::TM_HAS_VALUE_AND_RESULT;
    }


    PinM compRampVi100::pin_redirect_analog(const PinM& alias) const{
        return alias; 
    }
    PinM compRampVi100::pin_redirect_digital(const PinM& alias) const{
        return alias; 
    }
    void compRampVi100::set_vih(const PinM& pin, const FloatS& vih){
        DIGITAL.SetVih(pin,vih); 
    }
    void compRampVi100::set_vih(const PinML& pin, const FloatS1D& vih){
        DIGITAL.SetVih(pin,vih); 
    } 


    



    BoolM compRampVi100::Initialize()
    { 
        Profiler P(__PRETTY_FUNCTION__);
        
        //Move the ramp generation here!

        BoolM initialize_status = true;

       
        return initialize_status;
    }
 
    BoolM compRampVi100::ParamCheck()
    {
        StringS error_message;
        BoolM param_check_status = true;
        
        return param_check_status;
    }
    
    compRampVi100::compRampVi100()
    {
    
        AddInputParameter(ramp_type,"Ramp_Type","VI100_RAMP_TYPES:GATED");
        SetParameterAttribute("Ramp_Type","parameter-group","Method Config");
        SetParameterAttribute("Ramp_Type","tooltip",
            "Should the ramp be a gated ramp or syncronous triggered ramp?"
        );    
        AddInputParameter(monitor_in_delay,"monitor_in_delay","FALSE");
        SetParameterAttribute("monitor_in_delay","parameter-group","Method Config");
        SetParameterAttribute("monitor_in_delay","tooltip",
            "Should the input pin be measured for the delay with a TMU? Other delays will be reported relative to this one"
        );    
        AddInputParameter(capture_th_delay,"capture_th_delay","FALSE");
        SetParameterAttribute("capture_th_delay","parameter-group","Method Config");
        SetParameterAttribute("capture_th_delay","tooltip",
            "Should the delay to the threshold pin be reported?"
        );      
        AddInputParameter(capture_out_delay,"capture_out_delay","FALSE");
        SetParameterAttribute("capture_out_delay","parameter-group","Method Config");
        SetParameterAttribute("capture_out_delay","tooltip",
            "Should a second non-threshold digital pin be captured and the delay calculated? "
        );
        AddInputParameter(sync_ref,"sync_ref","''");
        SetParameterAttribute("sync_ref","parameter-group","Method Config");
        SetParameterAttribute("sync_ref","tooltip",
            "The sync ref for the analog trigger."
        );
        AddInputParameter(perform_connect,"perform_connect","FALSE");
        SetParameterAttribute("perform_connect","parameter-group","Method Config");
        SetParameterAttribute("perform_connect","tooltip",
            "Should instrument connect and gate mode be set before testing?"
        );
        AddInputParameter(perform_disconnect,"perform_disconnect","FALSE");
        SetParameterAttribute("perform_disconnect","parameter-group","Method Config");
        SetParameterAttribute("perform_disconnect","tooltip",
            "Should instrument disconnect and gate mode be set before testing?"
        );
        
        AddInputParameter(digital_pattern,"digital_pattern","''");
        SetParameterAttribute("digital_pattern","parameter-group","Pattern Config");
        SetParameterAttribute("digital_pattern","tooltip",
            "The digital pattern to run. The selected pins must report a fail count. "
        );
        
        AddInputParameter(digitze_samples,"digitze_samples","");
        SetParameterAttribute("digitze_samples","parameter-group","Pattern Config");
        SetParameterAttribute("digitze_samples","tooltip",
            "The number of samples to capture on the analog input. The repeat ratio between the analog and digital pins will be calculated based on the two sample rates."
        );
        
        AddInputParameter(timeout_samples,"timeout_samples","");
        SetParameterAttribute("timeout_samples","parameter-group","Pattern Config");
        SetParameterAttribute("timeout_samples","tooltip",
            "The total number of digital fails that should be captured before the pattern times out, this is in terms of the fail period."
        );
        
        AddInputParameter(max_sample_rate,"max_sample_rate","2MHz");
        SetParameterAttribute("max_sample_rate","parameter-group","Pattern Config");
        SetParameterAttribute("max_sample_rate","tooltip",
            "The max sample rate of the analog source"
        );
        
        AddInputParameter(fail_period,"fail_period","");
        SetParameterAttribute("fail_period","parameter-group","Pattern Config");
        SetParameterAttribute("fail_period","tooltip",
            "The time elapsed between fail counts in your pattern. This is used to calculate the delay."
        );
        AddInputParameter(hard_oversample,"hard_oversample","4");
        SetParameterAttribute("hard_oversample","parameter-group","Pattern Config");
        SetParameterAttribute("hard_oversample","tooltip",
            "This is the number of steps your pattern needs in the loop in addition to the RPT count"
        );
        
        AddInputParameter(input_pin,"input_pin","");
        SetParameterAttribute("input_pin","parameter-group","Ramp Setup");
        SetParameterAttribute("input_pin","tooltip",
            "The analog pin that will run the ramp."
        );
        
        AddInputParameter(start,"start","");
        SetParameterAttribute("start","parameter-group","Ramp Setup");
        SetParameterAttribute("start","tooltip",
            "The starting voltage or current. The start and stop must be the same unit type!"
        );
        AddInputParameter(stop,"stop","");
        SetParameterAttribute("stop","parameter-group","Ramp Setup");
        SetParameterAttribute("stop","tooltip",
            "The ending voltage or current. The start and stop must be the same unit type!"
        );
        AddInputParameter(force_steps,"force_steps","2");
        SetParameterAttribute("force_steps","parameter-group","Ramp Setup");
        SetParameterAttribute("force_steps","tooltip",
            "The number of steps in the analog pattern. For a 2-step gated pattern using the integrator bandwidth this would be equal to '2'"
        );
        AddInputParameter(force_bandwidth,"force_bandwidth","150kHz");
        SetParameterAttribute("force_bandwidth","parameter-group","Ramp Setup");
        SetParameterAttribute("force_bandwidth","tooltip",
            "The integrator bandwidth of the forcing instrument"
        );
        
        AddInputParameter(input_pin_sense,"input_pin_sense","");
        SetParameterAttribute("input_pin_sense","parameter-group","Input Sense Setup");
        SetParameterAttribute("input_pin_sense","display-if","..monitor_in_delay");
        SetParameterAttribute("input_pin_sense","tooltip",
            "The pinML that will be used to sense the threshold to start a timer. "
        );
        AddInputParameter(set_vih_in,"set_vih_in","");
        SetParameterAttribute("set_vih_in","parameter-group","Input Sense Setup");
        SetParameterAttribute("set_vih_in","display-if","..monitor_in_delay");
        SetParameterAttribute("set_vih_in","tooltip",
            "Should the VIH of the monitor digital be over-ridden?"
        );
        AddInputParameter(vih_in,"vih_in");
        SetParameterAttribute("vih_in","parameter-group","Input Sense Setup");
        SetParameterAttribute("vih_in","display-if","(..monitor_in_delay)AND(..set_vih_in)");
        SetParameterAttribute("vih_in","tooltip",
            "The VIH of the monitor pin"
        );
        
        
        AddInputParameter(output_pin_th,"output_pin_th","");
        SetParameterAttribute("output_pin_th","parameter-group","Threshold Sense Setup");
        SetParameterAttribute("output_pin_th","tooltip",
            "The pinML that will be used to sense the threshold.  "
        );
        AddInputParameter(set_vih_th,"set_vih_th","FALSE");
        SetParameterAttribute("set_vih_th","parameter-group","Threshold Sense Setup");
        SetParameterAttribute("set_vih_th","tooltip",
            "Should the VIH of the monitor digital be over-ridden?"
        );
        AddInputParameter(vih_th,"vih_th");
        SetParameterAttribute("vih_th","parameter-group","Threshold Sense Setup");
        SetParameterAttribute("vih_th","display-if","..set_vih_th");
        SetParameterAttribute("vih_th","tooltip",
            "The VIH of the monitor pin"
        );
        AddInputParameter(log_threshold_delay,"log_threshold_delay","FALSE");
        SetParameterAttribute("log_threshold_delay","parameter-group","Threshold Sense Setup");
        SetParameterAttribute("log_threshold_delay","tooltip",
            "Should the delay to the threshold be logged?"
        );
        
        
        AddInputParameter(output_pin_delay,"output_pin_delay","");
        SetParameterAttribute("output_pin_delay","parameter-group","Delay Sense Setup");
        SetParameterAttribute("output_pin_delay","display-if","..capture_out_delay");
        SetParameterAttribute("output_pin_delay","tooltip",
            "The pinML that will be used to sense the secondary delay."
        );
        AddInputParameter(set_vih_delay,"set_vih_delay","FALSE");
        SetParameterAttribute("set_vih_delay","parameter-group","Delay Sense Setup");
        SetParameterAttribute("set_vih_delay","display-if","..capture_out_delay");
        SetParameterAttribute("set_vih_delay","tooltip",
            "The pinML that will be used to sense the secondary delay."
        );
        AddInputParameter(vih_delay,"vih_delay");
        SetParameterAttribute("vih_delay","parameter-group","Delay Sense Setup");
        SetParameterAttribute("vih_delay","display-if","(..capture_out_delay)AND(..set_vih_delay)");
        SetParameterAttribute("vih_delay","tooltip",
            "The pinML that will be used to sense the secondary delay."
        );
        AddInputParameter(log_output_delay,"log_output_delay","FALSE");
        SetParameterAttribute("log_output_delay","parameter-group","Delay Sense Setup");
        SetParameterAttribute("log_output_delay","display-if","..capture_out_delay");
        SetParameterAttribute("log_output_delay","tooltip",
            "The pinML that will be used to sense the secondary delay."
        );
        
        AddOutputParameter(threshold,"threshold");
        SetParameterAttribute("threshold","parameter-group","Ouputs and Logging");
        SetParameterAttribute("threshold","tooltip",
            "The threshold where the trip occured."
        );
        AddInputParameter(ThresholdLimit,"ThresholdLimit","");
        SetParameterAttribute("ThresholdLimit","parameter-group","Ouputs and Logging");
        SetParameterAttribute("ThresholdLimit","tooltip",
            "The limit for the theshold"
        );
        AddOutputParameter(thDelay,"thDelay");
        SetParameterAttribute("thDelay","parameter-group","Ouputs and Logging");
        SetParameterAttribute("thDelay","display-if","..capture_th_delay");
        SetParameterAttribute("thDelay","tooltip",
            "The delay from input to threshold output. When no input sensing is used this is assumed to be from pattern start."
        );
        AddInputParameter(thDelayLimit,"thDelayLimit","");
        SetParameterAttribute("thDelayLimit","parameter-group","Ouputs and Logging");
        SetParameterAttribute("thDelayLimit","display-if","(..capture_th_delay)AND(..log_threshold_delay)");
        SetParameterAttribute("thDelayLimit","tooltip",
            "The threshold delay limit"
        );
        AddOutputParameter(OutDelay,"OutDelay");
        SetParameterAttribute("OutDelay","parameter-group","Ouputs and Logging");
        SetParameterAttribute("OutDelay","display-if","..capture_out_delay");
        SetParameterAttribute("OutDelay","tooltip",
            "The delay from input to secondary output. When no input sensing is used this is assumed to be from pattern start."
        );
        AddInputParameter(OutDelayLimit,"OutDelayLimit","");
        SetParameterAttribute("OutDelayLimit","parameter-group","Ouputs and Logging");
        SetParameterAttribute("OutDelayLimit","display-if","(..capture_out_delay)AND(..log_output_delay)");
        SetParameterAttribute("OutDelayLimit","tooltip",
            "The output delay limit"
        );

        
        
        
    

        
    }
    
//#ifdef ENABLE_REN_GENRIC
TMFAMILY_CLASS(REN::GeneralTests::compRampVi100)
//#endif 



}
}
