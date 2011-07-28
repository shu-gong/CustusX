#include "sscGraphicalPrimitives.h"

#include "boost/bind.hpp"
#include <vtkSphereSource.h>
#include <vtkLineSource.h>
#include <vtkArcSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkCommand.h>
#include <vtkFollower.h>
#include <vtkVectorText.h>
#include <vtkCamera.h>
#include "sscTypeConversions.h"
#include "sscBoundingBox3D.h"
#include "vtkArrowSource.h"
#include "vtkMatrix4x4.h"

namespace ssc
{

GraphicalPoint3D::GraphicalPoint3D(vtkRendererPtr renderer)
{
	mRenderer = renderer;
	source = vtkSphereSourcePtr::New();
	source->SetRadius(4);
//  default:
//  source->SetThetaResolution(8);
//  source->SetPhiResolution(8);
	// 24*16 = 384, 8*8=64, 16*12=192
	source->SetThetaResolution(16);
  source->SetPhiResolution(12);

	mapper = vtkPolyDataMapperPtr::New();
	mapper->SetInputConnection(source->GetOutputPort());

	actor = vtkActorPtr::New();
	actor->SetMapper(mapper);
	if (mRenderer)
	{
		mRenderer->AddActor(actor);
	}
}

GraphicalPoint3D::~GraphicalPoint3D()
{
	if (mRenderer)
	{
		mRenderer->RemoveActor(actor);
	}
}

void GraphicalPoint3D::setRadius(double radius)
{
	source->SetRadius(radius);
}

void GraphicalPoint3D::setColor(Vector3D color)
{
	actor->GetProperty()->SetColor(color.begin());
}

void GraphicalPoint3D::setValue(Vector3D point)
{
	actor->SetPosition(point.begin());
}

Vector3D GraphicalPoint3D::getValue() const
{
	return Vector3D(actor->GetPosition());
}

vtkActorPtr GraphicalPoint3D::getActor()
{
	return actor;
}


///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------


GraphicalLine3D::GraphicalLine3D( vtkRendererPtr renderer)
{
	mRenderer = renderer;
	source = vtkLineSourcePtr::New();
	mapper = vtkPolyDataMapperPtr::New() ;
	actor = vtkActorPtr::New() ;
	
	mapper->SetInputConnection( source->GetOutputPort() );
	actor->SetMapper (mapper );
	if (mRenderer)
		mRenderer->AddActor(actor);	
}

GraphicalLine3D::~GraphicalLine3D()
{
	if (mRenderer)
		mRenderer->RemoveActor(actor);
}

void GraphicalLine3D::setColor(Vector3D color)
{
	actor->GetProperty()->SetColor(color.begin());
}

void GraphicalLine3D::setValue(Vector3D point1, Vector3D point2)
{
	source->SetPoint1(point1.begin());
	source->SetPoint2(point2.begin());
}

void GraphicalLine3D::setStipple(int stipple)
{
	actor->GetProperty()->SetLineStipplePattern(stipple);
}

vtkActorPtr GraphicalLine3D::getActor()
{
	return actor;
}

///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------

///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------


GraphicalArc3D::GraphicalArc3D( vtkRendererPtr renderer)
{
  mRenderer = renderer;
  source = vtkArcSourcePtr::New();
  source->SetResolution(20);
  mapper = vtkPolyDataMapperPtr::New() ;
  actor = vtkActorPtr::New() ;

  mapper->SetInputConnection( source->GetOutputPort() );
  actor->SetMapper (mapper );
  if (mRenderer)
    mRenderer->AddActor(actor);
}

GraphicalArc3D::~GraphicalArc3D()
{
  if (mRenderer)
    mRenderer->RemoveActor(actor);
}

void GraphicalArc3D::setColor(Vector3D color)
{
  actor->GetProperty()->SetColor(color.begin());
}

void GraphicalArc3D::setValue(Vector3D point1, Vector3D point2, Vector3D center)
{
  source->SetPoint1(point1.begin());
  source->SetPoint2(point2.begin());
  source->SetCenter(center.begin());
}

void GraphicalArc3D::setStipple(int stipple)
{
  actor->GetProperty()->SetLineStipplePattern(stipple);
}

vtkActorPtr GraphicalArc3D::getActor()
{
  return actor;
}

///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------

///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------


GraphicalArrow3D::GraphicalArrow3D( vtkRendererPtr renderer)
{
  mRenderer = renderer;
  source = vtkArrowSourcePtr::New();
  source->SetTipResolution(24);
  source->SetShaftResolution(24);
  mapper = vtkPolyDataMapperPtr::New() ;
  actor = vtkActorPtr::New() ;

  mapper->SetInputConnection( source->GetOutputPort() );
  actor->SetMapper (mapper );
  if (mRenderer)
    mRenderer->AddActor(actor);
}

GraphicalArrow3D::~GraphicalArrow3D()
{
  if (mRenderer)
    mRenderer->RemoveActor(actor);
}

void GraphicalArrow3D::setColor(Vector3D color)
{
  actor->GetProperty()->SetColor(color.begin());
}

void GraphicalArrow3D::setValue(Vector3D base, Vector3D normal, double length)
{
	// find an arbitrary vector k perpendicular to normal:
	Vector3D k = cross(Vector3D(1,0,0), normal);
	if (similar(k, Vector3D(0,0,0)))
		k = cross(Vector3D(0,1,0), normal);
	k = k.normalized();
	Transform3D M =  createTransformIJC(normal, k, base);

//	std::cout << "GraphicalArrow3D::setValue  " << base << " - " << normal << std::endl;
	Transform3D S = createTransformScale(ssc::Vector3D(length,1,1));
	M =  M * S;
	// let arrow shape increase slowly with length:
	source->SetTipLength(0.35/sqrt(length));
	source->SetTipRadius(0.1*sqrt(length));
	source->SetShaftRadius(0.03*sqrt(length));
	actor->SetUserMatrix(M.getVtkMatrix());
}

///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------


Rect3D::Rect3D(vtkRendererPtr renderer, Vector3D color)
{
  mRenderer = renderer;
  mapper = vtkPolyDataMapperPtr::New();
  actor = vtkActorPtr::New();
  actor->GetProperty()->SetColor(color.begin());
  actor->SetMapper(mapper);
  if (mRenderer)
    mRenderer->AddActor(actor);

  mPolyData = vtkPolyDataPtr::New();
  mPoints = vtkPointsPtr::New();
  mSide = vtkCellArrayPtr::New();

  vtkIdType cells[5] = { 0,1,2,3,0 };
  mSide->InsertNextCell(5, cells);

  mPolyData->SetPoints(mPoints);
  mapper->SetInput(mPolyData);
}

void Rect3D::setLine(bool on, int width)
{
  if (on)
  {
    mPolyData->SetLines(mSide);
    actor->GetProperty()->SetLineWidth(width);
  }
  else
  {
    mPolyData->SetLines(NULL);
  }
}

void Rect3D::setSurface(bool on)
{
  if (on)
  {
    mPolyData->SetPolys(mSide);
    actor->GetProperty()->SetOpacity(1.0); // transparent planes dont work well together with texture volume. Use 1.0
  }
  else
  {
    mPolyData->SetPolys(NULL);
  }
}

Rect3D::~Rect3D()
{
  if (mRenderer)
    mRenderer->RemoveActor(actor);
}

void Rect3D::updatePosition(const DoubleBoundingBox3D bb, const Transform3D& M)
{
  mPoints = vtkPointsPtr::New();
  mPoints->InsertPoint(0, M.coord(bb.corner(0,0,0)).begin());
  mPoints->InsertPoint(1, M.coord(bb.corner(0,1,0)).begin());
  mPoints->InsertPoint(2, M.coord(bb.corner(1,1,0)).begin());
  mPoints->InsertPoint(3, M.coord(bb.corner(1,0,0)).begin());
  mPolyData->SetPoints(mPoints);
  mPolyData->Update();
}

///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------


FollowerText3D::FollowerText3D( vtkRendererPtr renderer)
{
  mRenderer = renderer;
  if (!mRenderer)
  	return;

  mViewportListener.reset(new ssc::ViewportListener);
	mViewportListener->setCallback(boost::bind(&FollowerText3D::scaleText, this));

  mText = vtkVectorText::New();
  vtkPolyDataMapperPtr mapper = vtkPolyDataMapperPtr::New();
  mapper->SetInput(mText->GetOutput());
  mFollower = vtkFollower::New();
  mFollower->SetMapper(mapper);
  mFollower->SetCamera(mRenderer->GetActiveCamera());
  ssc::Vector3D mTextScale(2,2,2);
  mFollower->SetScale(mTextScale.begin());

  mRenderer->AddActor(mFollower);
  this->setSizeInNormalizedViewport(true, 0.025);
}

FollowerText3D::~FollowerText3D()
{
  if (mRenderer)
    mRenderer->RemoveActor(mFollower);
}

void FollowerText3D::setSize(double val)
{
	mSize = val;
	this->scaleText();
}

void FollowerText3D::setSizeInNormalizedViewport(bool on, double size)
{
	if (on)
	{
		mViewportListener->startListen(mRenderer);
	}
	else
	{
		mViewportListener->stopListen();
	}

	this->setSize(size);
}

void FollowerText3D::setColor(Vector3D color)
{
	mFollower->GetProperty()->SetColor(color.begin());
}

void FollowerText3D::setText(QString text)
{
  mText->SetText(cstring_cast(text));
}

void FollowerText3D::setPosition(ssc::Vector3D pos)
{
  mFollower->SetPosition(pos.begin());
}

vtkFollowerPtr FollowerText3D::getActor()
{
  return mFollower;
}

/**Note: Internal method!
 *
 * Scale the text to be a constant fraction of the viewport height
 * Called from a vtk camera observer
 *
 */
void FollowerText3D::scaleText()
{
	if (!mViewportListener->isListening())
	{
    mFollower->SetScale(ssc::Vector3D(mSize,mSize,mSize).begin());
		return;
	}

	double size = mViewportListener->getVpnZoom();

  double scale = mSize/size;
//  std::cout << "s= " << size << "  ,scale= " << scale << std::endl;
  ssc::Vector3D mTextScale(scale,scale,scale);
  if (mFollower)
    mFollower->SetScale(mTextScale.begin());
}


///--------------------------------------------------------
///--------------------------------------------------------
///--------------------------------------------------------

}
