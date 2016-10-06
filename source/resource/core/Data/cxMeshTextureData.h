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

#ifndef MESHTEXTUREDATA_H
#define MESHTEXTUREDATA_H

#include "cxResourceExport.h"
#include "cxStringProperty.h"
#include "cxSelectDataStringProperty.h"


class QDomNode;

namespace cx
{

/**
 * Mesh texture data used in a vtkProperty
 */
class cxResource_EXPORT MeshTextureData : public QObject
{
	Q_OBJECT
public:
	MeshTextureData(PatientModelServicePtr patientModelService);
	void addXml(QDomNode& dataNode);
	void parseXml(QDomNode &dataNode);

	StringPropertyPtr getTextureShape() const;
	StringPropertySelectImagePtr getTextureImage() const;
	DoublePropertyPtr getScaleX() const;
	DoublePropertyPtr getScaleY() const;
	std::vector<PropertyPtr> getProperties() const;
	DoublePropertyPtr getPositionX() const;
	DoublePropertyPtr getPositionY() const;

	QString getCylinderText() const;
	QString getPlaneText() const;
	QString getSphereText() const;

signals:
	void changed();
private:
	void addProperty(PropertyPtr property);
	void initialize();
	std::vector<PropertyPtr> mProperties;
	StringPropertyPtr mTextureShape;
	StringPropertySelectImagePtr mTextureImage;
	DoublePropertyPtr mScaleX;
	DoublePropertyPtr mScaleY;
	DoublePropertyPtr mPositionX;
	DoublePropertyPtr mPositionY;
	PatientModelServicePtr mPatientModelService;
	QString mCylinderText;
	QString mPlaneText;
	QString mSphereText;
};

} // namespace cx

#endif // MESHTEXTUREDATA_H
