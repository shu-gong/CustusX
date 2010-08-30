/*
 * sscSlicePlaneClipper.cpp
 *
 *  Created on: Aug 20, 2010
 *      Author: christiana
 */

#include "sscSlicePlaneClipper.h"

#include <vector>
#include <vtkPlane.h>
#include <vtkVolume.h>
#include <vtkAbstractVolumeMapper.h>
#include <vtkPlaneCollection.h>

#include "sscSliceProxy.h"
#include "sscVolumetricRep.h"
#include "sscImage.h"

namespace ssc
{

SlicePlaneClipperPtr SlicePlaneClipper::New()
{
  return SlicePlaneClipperPtr(new SlicePlaneClipper());
}

SlicePlaneClipper::SlicePlaneClipper() :
  mInvertPlane(false)
{
}

SlicePlaneClipper::~SlicePlaneClipper()
{
  this->clearVolumes();
}

void SlicePlaneClipper::setSlicer(ssc::SliceProxyPtr slicer)
{
  if (mSlicer==slicer)
    return;
  if (mSlicer)
  {
    disconnect(mSlicer.get(), SIGNAL(transformChanged(Transform3D)), this, SLOT(changedSlot()));
  }
  mSlicer = slicer;
  if (mSlicer)
  {
    connect(mSlicer.get(), SIGNAL(transformChanged(Transform3D)), this, SLOT(changedSlot()));
  }

  this->updateClipPlane();
  for (VolumesType::iterator iter=mVolumes.begin(); iter!=mVolumes.end(); ++iter)
  {
    if (!(*iter)->getVtkVolume()->GetMapper()->GetClippingPlanes()->IsItemPresent(mClipPlane))
      (*iter)->getVtkVolume()->GetMapper()->AddClippingPlane(mClipPlane);
  }
  this->changedSlot();
}

ssc::SliceProxyPtr SlicePlaneClipper::getSlicer()
{
  return mSlicer;
}

void SlicePlaneClipper::clearVolumes()
{
//  std::cout << "SlicePlaneClipper::clearVolumes()" << std::endl;

  for (VolumesType::iterator iter=mVolumes.begin(); iter!=mVolumes.end(); ++iter)
  {
    (*iter)->getVtkVolume()->GetMapper()->RemoveClippingPlane(mClipPlane);
  }
  mVolumes.clear();
}

void SlicePlaneClipper::addVolume(ssc::VolumetricRepPtr volume)
{
  if (!volume)
    return;
  mVolumes.insert(volume);

  if (mClipPlane)
  {
    if (!volume->getVtkVolume()->GetMapper()->GetClippingPlanes()->IsItemPresent(mClipPlane))
      volume->getVtkVolume()->GetMapper()->AddClippingPlane(mClipPlane);
  }

  this->changedSlot();
}

void SlicePlaneClipper::removeVolume(ssc::VolumetricRepPtr volume)
{
  if (!volume)
    return;
  volume->getVtkVolume()->GetMapper()->RemoveClippingPlane(mClipPlane);
  mVolumes.erase(volume);
  this->changedSlot();
}

SlicePlaneClipper::VolumesType SlicePlaneClipper::getVolumes()
{
  return mVolumes;
}

void SlicePlaneClipper::setInvertPlane(bool on)
{
  mInvertPlane = on;
  changedSlot();
}

bool SlicePlaneClipper::getInvertPlane() const
{
  return mInvertPlane;
}

/** return an untransformed plane normal to use during clipping.
 *  The direction is dependent in invertedPlane()
 */
ssc::Vector3D SlicePlaneClipper::getUnitNormal() const
{
  if (mInvertPlane)
    return ssc::Vector3D(0,0,1);
  else
    return ssc::Vector3D(0,0,-1);
}

/** return a vtkPlane representing the current clip plane.
 */
vtkPlanePtr SlicePlaneClipper::getClipPlaneCopy()
{
  vtkPlanePtr retval = vtkPlanePtr::New();
  retval->SetNormal(mClipPlane->GetNormal());
  retval->SetOrigin(mClipPlane->GetOrigin());
  return retval;
}

void SlicePlaneClipper::updateClipPlane()
{
  if (!mSlicer)
    return;
  if (!mClipPlane)
    mClipPlane = vtkPlanePtr::New();

  ssc::Transform3D rMs = mSlicer->get_sMr().inv();

  ssc::Vector3D n = rMs.vector(this->getUnitNormal());
  ssc::Vector3D p = rMs.coord(ssc::Vector3D(0,0,0));
  mClipPlane->SetNormal(n.begin());
  mClipPlane->SetOrigin(p.begin());
}

void SlicePlaneClipper::changedSlot()
{
  if (!mSlicer)
    return;

  this->updateClipPlane();
  //std::cout << "SlicePlaneClipper::changedSlot()" << std::endl;
}



///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------



ImageMapperMonitor::ImageMapperMonitor(ssc::VolumetricRepPtr volume) : mVolume(volume), mImage(volume->getImage())
{
  if (!mImage)
    return;

  //std::cout << "ImageMapperMonitor::ImageMapperMonitor()" << std::endl;
  connect(mImage.get(), SIGNAL(clipPlanesChanged()), this, SLOT(clipPlanesChangedSlot()));
  this->fillClipPlanes();
}

ImageMapperMonitor::~ImageMapperMonitor()
{
  this->clearClipPlanes();
}

void ImageMapperMonitor::clipPlanesChangedSlot()
{
  //std::cout << "ImageMapperMonitor::clipPlanesChangedSlot()" << std::endl;
  this->clearClipPlanes();
  this->fillClipPlanes();
}

void ImageMapperMonitor::clearClipPlanes()
{
  if (!mImage)
    return;

 // std::vector<vtkPlanePtr> planes = mImage->getClipPlanes();
  for (unsigned i=0; i<mPlanes.size(); ++i)
  {
    //std::cout << "ImageMapperMonitor::clearClipPlanes(" << i << ")" << std::endl;
    mVolume->getVtkVolume()->GetMapper()->RemoveClippingPlane(mPlanes[i]);
  }
  mPlanes.clear();
}

void ImageMapperMonitor::fillClipPlanes()
{
  if (!mImage)
    return;

  mPlanes = mImage->getClipPlanes();
  for (unsigned i=0; i<mPlanes.size(); ++i)
  {
    //std::cout << "ImageMapperMonitor::fillClipPlanes(" << i << ")" << std::endl;
    mVolume->getVtkVolume()->GetMapper()->AddClippingPlane(mPlanes[i]);
  }
}



} // namespace ssc
