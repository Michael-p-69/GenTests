#pragma once
#include <Unison.h>
#include <CoreFamily.h>
#include <map>
#include "enums.h"

namespace REN{
namespace GeneralTests{

class logDelta : public LTXC::CoreFamily
{
 
 public:
    logDelta();
    virtual ~logDelta() { }
    
    TM_RETURN        Execute();
    BoolM            ParamCheck();
    BoolM            Initialize();
 

protected:

    ArrayOfFloatM LHS; 
    ArrayOfFloatM RHS; 
    ArrayOfObjectS  Limits; 
    
    
private:
 
};

}
}
