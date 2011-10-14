/*
 * sscSlices3DRep.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: christiana
 */

#include <sscSlices3DRep.h>

#include <vtkRenderer.h>
#include <vtkFloatArray.h>
#include <vtkPlaneSource.h>
#include <vtkPointData.h>
#include <vtkTriangleFilter.h>
#include <vtkStripper.h>
#include <vtkImageData.h>
#include <vtkPainterPolyDataMapper.h>
#include <vtkLookupTable.h>

#include "sscImage.h"
#include "sscView.h"
#include "sscImageLUT2D.h"
#include "sscSliceProxy.h"
#include "sscTypeConversions.h"
#include "sscGPUImageBuffer.h"
#include "sscTexture3DSlicerProxy.h"

//---------------------------------------------------------
namespace ssc
{
//---------------------------------------------------------

Slices3DRep::Slices3DRep(const QString& uid) :
	RepImpl(uid)
{
//	mProxy = Texture3DSlicerProxy::New();
//	mProxy->setTargetSpaceToR();
	mView = NULL;
}

Slices3DRep::~Slices3DRep()
{
}

Slices3DRepPtr Slices3DRep::New(const QString& uid)
{
	Slices3DRepPtr retval(new Slices3DRep(uid));
    retval->mSelf = retval;
    return retval;
}

void Slices3DRep::setShaderFile(QString shaderFile)
{
	for (unsigned i=0; i<mProxy.size(); ++i)
		mProxy[i]->setShaderFile(shaderFile);
}

void Slices3DRep::setImages(std::vector<ssc::ImagePtr> images)
{
	for (unsigned i=0; i<mProxy.size(); ++i)
	{
		mProxy[i]->setImages(images);
		mProxy[i]->getSliceProxy()->setDefaultCenter(images[0]->get_rMd().coord(images[0]->boundingBox().center()));
	}
}

void Slices3DRep::addPlane(PLANE_TYPE plane)
{
	ssc::SliceProxyPtr sliceProxy = ssc::SliceProxy::New("");
	sliceProxy->initializeFromPlane(ssc::ptAXIAL, false, ssc::Vector3D(0,0,1), true, 150, 0.25);


	Texture3DSlicerProxyPtr current = Texture3DSlicerProxy::New();
	sliceProxy->initializeFromPlane(plane, false, ssc::Vector3D(0,0,1), true, 150, 0.25);
	sliceProxy->setAlwaysUseDefaultCenter(true);
	current->setSliceProxy(sliceProxy);
	current->setTargetSpaceToR();

	mProxy.push_back(current);
}

void Slices3DRep::setTool(ToolPtr tool)
{
	for (unsigned i=0; i<mProxy.size(); ++i)
		mProxy[i]->getSliceProxy()->setTool(tool);
}


//void Slices3DRep::setSliceProxy(ssc::SliceProxyPtr slicer)
//{
//	mProxy->setSliceProxy(slicer);
//}

void Slices3DRep::addRepActorsToViewRenderer(ssc::View* view)
{
	for (unsigned i=0; i<mProxy.size(); ++i)
		view->getRenderer()->AddActor(mProxy[i]->getActor());
    mView = view;
//    connect(view, SIGNAL(resized(QSize)), this, SLOT(viewChanged()));
//    this->viewChanged();
}

void Slices3DRep::removeRepActorsFromViewRenderer(ssc::View* view)
{
	for (unsigned i=0; i<mProxy.size(); ++i)
		view->getRenderer()->RemoveActor(mProxy[i]->getActor());
//	disconnect(view, SIGNAL(resized(QSize)), this, SLOT(viewChanged()));
	mView = NULL;
}

//---------------------------------------------------------
}//end namespace
//---------------------------------------------------------
