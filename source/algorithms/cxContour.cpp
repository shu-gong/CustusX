#include "cxContour.h"

#include <vtkImageShrink3D.h>
#include <vtkMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>

#include "sscMessageManager.h"
#include "sscDataManager.h"
#include "sscUtilHelpers.h"
#include "sscMesh.h"
#include "sscRegistrationTransform.h"
#include "sscTypeConversions.h"

namespace cx
{
Contour::Contour() :
    TimedAlgorithm("centerline", 5)
{
  connect(&mWatcher, SIGNAL(finished()), this, SLOT(finishedSlot()));
}

Contour::~Contour()
{}

void Contour::setInput(ssc::ImagePtr image, QString outputBasePath, int threshold, double decimation, bool reduceResolution, bool smoothing)
{
  mInput = image;
  mOutputBasePath = outputBasePath;
  mThreshold = threshold;
  mDecimation = decimation;
  mUseReduceResolution = reduceResolution;
  mUseSmoothing = smoothing;


  this->generate();
}

ssc::MeshPtr Contour::getOutput()
{
  return mOutput;
}

void Contour::generate()
{
  this->startTiming();

  mFutureResult = QtConcurrent::run(this, &Contour::calculate);
  mWatcher.setFuture(mFutureResult);
}

void Contour::finishedSlot()
{
  vtkPolyDataPtr cubesPolyData = mWatcher.future().result();
  if(!cubesPolyData)
  {
    ssc::messageManager()->sendError("Centerline extraction failed.");
    return;
  }

  QString uid = ssc::changeExtension(mInput->getUid(), "") + "_cont%1";
  QString name = mInput->getName() + " contour %1";
  //std::cout << "contoured volume: " << uid << ", " << name << std::endl;
  mOutput = ssc::dataManager()->createMesh(cubesPolyData, uid, name, "Images");
  //ssc::messageManager()->sendInfo("Created contour " + mOutput->getName());

  mOutput->get_rMd_History()->setRegistration(mInput->get_rMd());
  mOutput->get_rMd_History()->addParentFrame(mInput->getUid());

  ssc::dataManager()->loadData(mOutput);
  ssc::dataManager()->saveMesh(mOutput, mOutputBasePath);

  this->stopTiming();
  ssc::messageManager()->sendSuccess("Created contour \"" + mOutput->getName()+"\"");

  emit finished();
}

//ssc::MeshPtr SegmentationOld::contour(ssc::ImagePtr image, QString mOutputBasePath, int mThreshold, double mDecimation, bool mUseReduceResolution, bool mUseSmoothing)
vtkPolyDataPtr Contour::calculate()
{
  ssc::messageManager()->sendDebug("Contour, mThreshold: "+qstring_cast(mThreshold)+", mDecimation: "+qstring_cast(mDecimation)+", reduce resolution: "+qstring_cast(mUseReduceResolution)+", mUseSmoothing: "+qstring_cast(mUseSmoothing));

  //itkImageType::ConstPointer itkImage = getITKfromSSCImageOld(image);

    //Create vtkPolyData
  /*vtkImageToPolyDataFilter* convert = vtkImageToPolyDataFilter::New();
   convert->SetInput(itkToVtkFilter->GetOutput());
   convert->SetColorModeToLinear256();
   convert->Update();*/

  //Shrink input volume
  vtkImageShrink3DPtr shrinker = vtkImageShrink3DPtr::New();
  if(mUseReduceResolution)
  {
    //ssc::messageManager()->sendInfo("Shrinking volume to be contoured...");
    shrinker->SetInput(mInput->getBaseVtkImageData());
    shrinker->SetShrinkFactors(2,2,2);
    shrinker->Update();
  }

  // Find countour
  //ssc::messageManager()->sendInfo("Finding surface shape...");
  vtkMarchingCubesPtr convert = vtkMarchingCubesPtr::New();
  if(mUseReduceResolution)
    convert->SetInput(shrinker->GetOutput());
  else
    convert->SetInput(mInput->getBaseVtkImageData());
  //convert->ComputeNormalsOn();
  convert->SetValue(0, mThreshold);
  //convert->SetValue(0, 1);
  convert->Update();
  //messageManager()->sendInfo("Number of contours: "+QString::number(convert->GetNumberOfContours()).toStdString());

  vtkPolyDataPtr cubesPolyData = vtkPolyDataPtr::New();
  cubesPolyData = convert->GetOutput();
  //cubesPolyData->DeepCopy(convert->GetOutput());

  // Smooth surface model
  vtkWindowedSincPolyDataFilterPtr smoother = vtkWindowedSincPolyDataFilterPtr::New();
  if(mUseSmoothing)
  {
    //ssc::messageManager()->sendInfo("Smoothing surface...");
    smoother->SetInput(cubesPolyData);
    smoother->Update();
    cubesPolyData = smoother->GetOutput();
  }

  //Create a surface of triangles

  //Decimate surface model (remove a percentage of the polygons)
  vtkTriangleFilterPtr trifilt = vtkTriangleFilterPtr::New();
  vtkDecimateProPtr deci = vtkDecimateProPtr::New();
  vtkPolyDataNormalsPtr normals = vtkPolyDataNormalsPtr::New();
  if (mDecimation > 0.000001)
  {
    //ssc::messageManager()->sendInfo("Creating surface triangles...");
    trifilt->SetInput(cubesPolyData);
    trifilt->Update();
    //ssc::messageManager()->sendInfo("Decimating surface...");
    deci->SetInput(trifilt->GetOutput());
    deci->SetTargetReduction(mDecimation);
    deci->PreserveTopologyOff();
    deci->Update();
    cubesPolyData = deci->GetOutput();
  }

  normals->SetInput(cubesPolyData);
  normals->Update();

  cubesPolyData->DeepCopy(normals->GetOutput());

  return cubesPolyData;
}
}//namespace cx
