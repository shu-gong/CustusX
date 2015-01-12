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

#include <cxBoundingBoxWidget.h>
#include "cxBoundingBox3D.h"
#include "cxDoubleSpanSlider.h"
#include "cxDoublePairProperty.h"

namespace cx
{

BoundingBoxWidget::BoundingBoxWidget(QWidget* parent) :
				QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);

	QStringList caption;
	caption << "X (mm)" << "Y (mm)" << "Z (mm)";

	for (int i=0; i<caption.size(); ++i)
	{
		DoublePairDataAdapterXmlPtr dataAdapter = DoublePairDataAdapterXml::initialize(caption[i], caption[i], caption[i], DoubleRange(-2000, 2000, 1), 0);
		mRange[i] = new SliderRangeGroupWidget(this, dataAdapter);
		connect(mRange[i], SIGNAL(valueChanged(double,double)), this, SIGNAL(changed()));
		layout->addWidget(mRange[i]);
	}
}

void BoundingBoxWidget::showDim(int dim, bool visible)
{
	mRange[dim]->setVisible(visible);
}

void BoundingBoxWidget::setValue(const DoubleBoundingBox3D& value, const DoubleBoundingBox3D& range)
{
	for (int i=0; i<3; ++i)
	{
		mRange[i]->blockSignals(true);
		mRange[i]->setRange(DoubleRange(range.begin()[2*i], range.begin()[2*i+1], 1));
		mRange[i]->setValue(value.begin()[2*i], value.begin()[2*i+1]);
		mRange[i]->blockSignals(false);
	}
}

DoubleBoundingBox3D BoundingBoxWidget::getValue() const
{
	std::pair<double, double> x = mRange[0]->getValue();
	std::pair<double, double> y = mRange[1]->getValue();
	std::pair<double, double> z = mRange[2]->getValue();
	DoubleBoundingBox3D box(x.first, x.second, y.first, y.second, z.first, z.second);
	return box;
}

}
