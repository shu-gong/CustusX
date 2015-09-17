/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
   may be used to endorse or promote products derived from this software 
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "cxShadingParamsInterfaces.h"
#include "cxImage.h"
#include "cxPatientModelService.h"

namespace cx
{
DoublePropertyShadingBase::DoublePropertyShadingBase(PatientModelServicePtr patientModelService) :
	mPatientModelService(patientModelService)
{
	mActiveImageProxy = ActiveImageProxy::New(patientModelService);
	connect(mActiveImageProxy.get(), &ActiveImageProxy::activeImageChanged, this, &DoublePropertyShadingBase::activeImageChanged);
	connect(mActiveImageProxy.get(), &ActiveImageProxy::transferFunctionsChanged, this, &Property::changed);
}

void DoublePropertyShadingBase::activeImageChanged()
{  
  mImage = mPatientModelService->getActiveData<Image>();
  emit changed();
}

DoublePropertyShadingAmbient::DoublePropertyShadingAmbient(PatientModelServicePtr patientModelService) :
	DoublePropertyShadingBase(patientModelService)
{
}

double DoublePropertyShadingAmbient::getValue() const
{
  if (!mImage)
    return 0.0;
  return mImage->getShadingAmbient();
}

bool DoublePropertyShadingAmbient::setValue(double val)
{ 
  if (!mImage)
    return false;
  if (similar(val, mImage->getShadingAmbient()))
    return false;
  mImage->setShadingAmbient(val);
  return true;
}


DoublePropertyShadingDiffuse::DoublePropertyShadingDiffuse(PatientModelServicePtr patientModelService) :
	DoublePropertyShadingBase(patientModelService)
{

}

double DoublePropertyShadingDiffuse::getValue() const
{
  if (!mImage)
    return 0.0;
  return mImage->getShadingDiffuse();
}
bool DoublePropertyShadingDiffuse::setValue(double val)
{ 
  if (!mImage)
    return false;
  if (similar(val, mImage->getShadingDiffuse()))
    return false;
  mImage->setShadingDiffuse(val);
  return true;
}


DoublePropertyShadingSpecular::DoublePropertyShadingSpecular(PatientModelServicePtr patientModelService) :
	DoublePropertyShadingBase(patientModelService)
{

}

double DoublePropertyShadingSpecular::getValue() const
{ 
  if (!mImage)
    return 0.0;
  return mImage->getShadingSpecular();
}
bool DoublePropertyShadingSpecular::setValue(double val)
{ 
  if (!mImage)
    return false;
  if (similar(val, mImage->getShadingSpecular()))
    return false;
  mImage->setShadingSpecular(val);
  return true;
}


DoublePropertyShadingSpecularPower::DoublePropertyShadingSpecularPower(PatientModelServicePtr patientModelService) :
	DoublePropertyShadingBase(patientModelService)
{
}

double DoublePropertyShadingSpecularPower::getValue() const
{ 
  if (!mImage)
    return 0.0;
  return mImage->getShadingSpecularPower();
}
bool DoublePropertyShadingSpecularPower::setValue(double val)
{ 
  if (!mImage)
    return false;
  if (similar(val, mImage->getShadingSpecularPower()))
    return false;
  mImage->setShadingSpecularPower(val);
  return true;
}

  
}// namespace cx

