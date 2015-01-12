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

#include "cxPrepareVesselsWidget.h"

#include <QVBoxLayout>
#include <QPushButton>
#include "cxLabeledComboBoxWidget.h"
#include "cxTypeConversions.h"
#include "cxColorSelectButton.h"
#include "cxMesh.h"
#include "cxDataLocations.h"

#include "cxPipelineWidget.h"
#include "cxHelperWidgets.h"
#include "cxColorProperty.h"

#include "cxResampleImageFilter.h"
#include "cxSmoothingImageFilter.h"
#include "cxBinaryThinningImageFilter3DFilter.h"
#include "cxBinaryThresholdImageFilter.h"
#include "cxRegistrationServiceProxy.h"

namespace cx
{
//------------------------------------------------------------------------------
PrepareVesselsWidget::PrepareVesselsWidget(RegServices services, QWidget* parent) :
		RegistrationBaseWidget(services, parent, "PrepareVesselsWidget", "PrepareVesselsWidget")
{  
	XmlOptionFile options = XmlOptionFile(DataLocations::getXmlSettingsFile()).descend("registration").descend("PrepareVesselsWidget");
  // fill the pipeline with filters:
	mPipeline.reset(new Pipeline(services.patientModelService));
  FilterGroupPtr filters(new FilterGroup(options.descend("pipeline")));
	filters->append(FilterPtr(new ResampleImageFilter(services.patientModelService)));
	filters->append(FilterPtr(new SmoothingImageFilter(services.patientModelService)));
	filters->append(FilterPtr(new BinaryThresholdImageFilter(services.patientModelService)));
	filters->append(FilterPtr(new BinaryThinningImageFilter3DFilter(services.patientModelService)));
  mPipeline->initialize(filters);

//  mPipeline->getNodes()[0]->setValueName("US Image:");
//  mPipeline->getNodes()[0]->setHelp("Select an US volume acquired from the wire phantom.");
  mPipeline->setOption("Color", QVariant(QColor("red")));

  mLayout = new QVBoxLayout(this);

  mPipelineWidget = new PipelineWidget(services.visualizationService, services.patientModelService, NULL, mPipeline);
  mLayout->addWidget(mPipelineWidget);

  mColorDataAdapter = ColorDataAdapterXml::initialize("Color", "",
                                              "Color of all generated data.",
                                              QColor("green"), options.getElement());
  connect(mColorDataAdapter.get(), SIGNAL(changed()), this, SLOT(setColorSlot()));

  QPushButton* fixedButton = new QPushButton("Set as Fixed");
  fixedButton->setToolTip("Set output of centerline generation as the Fixed Volume in Registration");
  connect(fixedButton, SIGNAL(clicked()), this, SLOT(toFixedSlot()));
  QPushButton* movingButton = new QPushButton("Set as Moving");
  movingButton->setToolTip("Set output of centerline generation as the Moving Volume in Registration");
  connect(movingButton, SIGNAL(clicked()), this, SLOT(toMovingSlot()));

  QLayout* buttonsLayout = new QHBoxLayout;
  buttonsLayout->addWidget(fixedButton);
  buttonsLayout->addWidget(movingButton);

	mLayout->addWidget(sscCreateDataWidget(this, mColorDataAdapter));
  mLayout->addWidget(mPipelineWidget);
  mLayout->addStretch();
  mLayout->addLayout(buttonsLayout);
//  mLayout->addStretch();

  this->setColorSlot();

}

void PrepareVesselsWidget::setColorSlot()
{
  mPipeline->setOption(mColorDataAdapter->getDisplayName(), QVariant(mColorDataAdapter->getValue()));
}

void PrepareVesselsWidget::toMovingSlot()
{
	DataPtr data = mPipeline->getNodes().back()->getData();
	if (data)
		mServices.registrationService->setMovingData(data);
}

void PrepareVesselsWidget::toFixedSlot()
{
	DataPtr data = mPipeline->getNodes().back()->getData();
	if (data)
		mServices.registrationService->setFixedData(data);
}

PrepareVesselsWidget::~PrepareVesselsWidget()
{}

QString PrepareVesselsWidget::defaultWhatsThis() const
{
  return "<html>"
      "<h3>Segmentation and centerline extraction for the i2i registration.</h3>"
      "<p><i>Segment out blood vessels from the selected image, then extract the centerline."
      "When finished, set the result as moving or fixed data in the registration tab.</i></p>"
      "<p><b>Tip:</b> The centerline extraction can take a <b>long</b> time.</p>"
      "</html>";
}

//------------------------------------------------------------------------------
}//namespace cx
