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


#include "cxDataManagerImpl.h"

#include <QtCore>
#include <QDomDocument>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>

#include "cxTransform3D.h"
#include "cxRegistrationTransform.h"
#include "cxLogger.h"
#include "cxTypeConversions.h"
#include "cxUtilHelpers.h"
#include "cxVideoSource.h"
#include "cxDataLocations.h"

#include "cxImageLUT2D.h"
#include "cxImageTF3D.h"

#include "cxDataReaderWriter.h"
#include "cxSpaceProvider.h"
#include "cxDataFactory.h"

#include "cxXmlOptionItem.h"
#include "cxTransferFunctions3DPresets.h"
#include "cxProfile.h"
#include "cxSettings.h"
#include "cxDefinitionStrings.h"


namespace cx
{

DataManagerImplPtr DataManagerImpl::create()
{
	DataManagerImplPtr retval;
	retval.reset(new DataManagerImpl());
	return retval;
}

DataManagerImpl::DataManagerImpl()
{
	m_rMpr_History.reset(new RegistrationHistory());
	connect(m_rMpr_History.get(), SIGNAL(currentChanged()), this, SIGNAL(rMprChanged()));
	mPatientLandmarks = Landmarks::create();

	connect(settings(), SIGNAL(valueChangedFor(QString)), this, SLOT(settingsChangedSlot(QString)));
	this->readClinicalView();

	this->clear();
}

DataManagerImpl::~DataManagerImpl()
{
}

void DataManagerImpl::setSpaceProvider(SpaceProviderPtr spaceProvider)
{
	mSpaceProvider = spaceProvider;
}

void DataManagerImpl::setDataFactory(DataFactoryPtr dataFactory)
{
	mDataFactory = dataFactory;
}

SpaceProviderPtr DataManagerImpl::getSpaceProvider()
{
	return mSpaceProvider;
}

DataFactoryPtr DataManagerImpl::getDataFactory()
{
	return mDataFactory;
}

void DataManagerImpl::clear()
{
	mData.clear();
	mCenter = Vector3D(0, 0, 0);
	mActiveImage.reset();
	mLandmarkProperties.clear();

	m_rMpr_History->clear();
	mPatientLandmarks->clear();

	emit centerChanged();
	emit activeImageChanged("");
	emit landmarkPropertiesChanged();
	emit dataAddedOrRemoved();
}

LandmarksPtr DataManagerImpl::getPatientLandmarks()
{
	return mPatientLandmarks;
}

// streams
VideoSourcePtr DataManagerImpl::getStream(const QString& uid) const
{
	if (mStreams.count(uid))
		return mStreams.find(uid)->second;
	return VideoSourcePtr();
}

DataManager::StreamMap DataManagerImpl::getStreams() const
{
	return mStreams;
}

void DataManagerImpl::loadStream(VideoSourcePtr stream)
{
	if (!stream)
		return;
	mStreams[stream->getUid()] = stream;
	emit streamLoaded();
}

std::map<QString, VideoSourcePtr> mStreams;

Vector3D DataManagerImpl::getCenter() const
{
	return mCenter;
}
void DataManagerImpl::setCenter(const Vector3D& center)
{
	mCenter = center;
	emit centerChanged();
}

ImagePtr DataManagerImpl::getActiveImage() const
{
	return mActiveImage;
}
void DataManagerImpl::setActiveImage(ImagePtr activeImage)
{
	if (mActiveImage == activeImage)
		return;

	mActiveImage = activeImage;

	QString uid = "";
	if (mActiveImage)
		uid = mActiveImage->getUid();

	emit activeImageChanged(uid);
//	report("Active image set to " + qstring_cast(uid));
}

void DataManagerImpl::setLandmarkNames(std::vector<QString> names)
{
	mLandmarkProperties.clear();
	for (unsigned i = 0; i < names.size(); ++i)
	{
		LandmarkProperty prop(qstring_cast(i + 1), names[i]); // generate 1-indexed uids (keep users happy)
		mLandmarkProperties[prop.getUid()] = prop;
	}
	emit landmarkPropertiesChanged();
}

QString DataManagerImpl::addLandmark()
{
	int max = 0;
	std::map<QString, LandmarkProperty>::iterator iter;
	for (iter = mLandmarkProperties.begin(); iter != mLandmarkProperties.end(); ++iter)
	{
		//max = std::max(max, qstring_cast(iter->second.getName()).toInt());
		max = std::max(max, qstring_cast(iter->first).toInt());
	}
	QString uid = qstring_cast(max + 1);
	mLandmarkProperties[uid] = LandmarkProperty(uid);

	emit landmarkPropertiesChanged();
	return uid;
}

void DataManagerImpl::setLandmarkName(QString uid, QString name)
{
	mLandmarkProperties[uid].setName(name);
	emit landmarkPropertiesChanged();
}

std::map<QString, LandmarkProperty> DataManagerImpl::getLandmarkProperties() const
{
	return mLandmarkProperties;
}

void DataManagerImpl::setLandmarkActive(QString uid, bool active)
{
	mLandmarkProperties[uid].setActive(active);
	emit landmarkPropertiesChanged();
}

DataPtr DataManagerImpl::loadData(const QString& uid, const QString& path)
{
	if (mData.count(uid)) // dont load same image twice
		return mData[uid];

	QString type = DataReaderWriter().findDataTypeFromFile(path);
	DataPtr data = mDataFactory->create(type, uid);
	bool loaded = data->load(path);

	if (!data || !loaded)
	{
		reportError("Error with data file: " + path);
		return DataPtr();
	}
	this->loadData(data);
	return data;
}

void DataManagerImpl::loadData(DataPtr data)
{
	if (data->getUid().contains('%'))
	{
		QString uid = data->getUid();
		QString name = data->getName();
		this->generateUidAndName(&uid, &name);
		data->setName(name);
		data->setUid(uid);
	}

	if (data)
	{
		if (mData.count(data->getUid()) && mData[data->getUid()]!=data)
			reportError(QString("Overwriting Data with uid=%1 with new object into PasM").arg(data->getUid()));
//		this->verifyParentFrame(data);
		mData[data->getUid()] = data;
		emit dataAddedOrRemoved();
	}
}

DataPtr DataManagerImpl::getData(const QString& uid) const
{
	DataMap::const_iterator iter = mData.find(uid);
	if (iter == mData.end())
		return DataPtr();
	return iter->second;
}

std::map<QString, DataPtr> DataManagerImpl::getData() const
{
	return mData;
}

std::map<QString, ImagePtr> DataManagerImpl::getImages() const
{
	std::map<QString, ImagePtr> retval;
	for (DataMap::const_iterator iter = mData.begin(); iter != mData.end(); ++iter)
	{
		ImagePtr image = this->getImage(iter->first);
		if (!image)
			continue;
		retval[iter->first] = image;
	}
	return retval;
}

std::map<QString, MeshPtr> DataManagerImpl::getMeshes() const
{
	std::map<QString, MeshPtr> retval;
	for (DataMap::const_iterator iter = mData.begin(); iter != mData.end(); ++iter)
	{
		MeshPtr mesh = this->getMesh(iter->first);
		if (!mesh)
			continue;
		retval[iter->first] = mesh;
	}
	return retval;
}

void DataManagerImpl::addXml(QDomNode& parentNode)
{
	QDomDocument doc = parentNode.ownerDocument();
	QDomElement dataManagerNode = doc.createElement("datamanager");
	parentNode.appendChild(dataManagerNode);

	m_rMpr_History->addXml(dataManagerNode);

	QDomElement activeImageNode = doc.createElement("activeImageUid");
	if (mActiveImage)
		activeImageNode.appendChild(doc.createTextNode(mActiveImage->getUid()));
	dataManagerNode.appendChild(activeImageNode);

	QDomElement landmarkPropsNode = doc.createElement("landmarkprops");
	LandmarkPropertyMap::iterator it = mLandmarkProperties.begin();
	for (; it != mLandmarkProperties.end(); ++it)
	{
		QDomElement landmarkPropNode = doc.createElement("landmarkprop");
		it->second.addXml(landmarkPropNode);
		landmarkPropsNode.appendChild(landmarkPropNode);
	}
	dataManagerNode.appendChild(landmarkPropsNode);

	QDomElement landmarksNode = doc.createElement("landmarks");
	mPatientLandmarks->addXml(landmarksNode);
	dataManagerNode.appendChild(landmarksNode);

	QDomElement centerNode = doc.createElement("center");
	centerNode.appendChild(doc.createTextNode(qstring_cast(mCenter)));
	dataManagerNode.appendChild(centerNode);

	for (DataMap::const_iterator iter = mData.begin(); iter != mData.end(); ++iter)
	{
		QDomElement dataNode = doc.createElement("data");
		dataManagerNode.appendChild(dataNode);
		iter->second->addXml(dataNode);
	}
}

void DataManagerImpl::parseXml(QDomNode& dataManagerNode, QString rootPath)
{
	// look in the toolmanager, for backwards compatibility (2014-02-21)
	QDomNode toolManagerNode = dataManagerNode.parentNode().namedItem("toolManager");

	QDomNode registrationHistory = dataManagerNode.namedItem("registrationHistory");
	if (registrationHistory.isNull())
		registrationHistory = toolManagerNode.namedItem("registrationHistory");
	m_rMpr_History->parseXml(registrationHistory);

	QDomNode landmarksNode = dataManagerNode.namedItem("landmarkprops");
	QDomElement landmarkNode = landmarksNode.firstChildElement("landmarkprop");
	for (; !landmarkNode.isNull(); landmarkNode = landmarkNode.nextSiblingElement("landmarkprop"))
	{
		LandmarkProperty landmarkProp;
		landmarkProp.parseXml(landmarkNode);
		mLandmarkProperties[landmarkProp.getUid()] = landmarkProp;
		//std::cout << "Loaded landmarkprop with name: " << landmarkProp.getName() << std::endl;
		emit landmarkPropertiesChanged();
	}

	QDomNode patientLandmarksNode = dataManagerNode.namedItem("landmarks");
	if (patientLandmarksNode.isNull())
		patientLandmarksNode = toolManagerNode.namedItem("landmarks");
	mPatientLandmarks->parseXml(patientLandmarksNode);

	// All images must be created from the DataManager, so the image nodes are parsed here
	std::map<DataPtr, QDomNode> datanodes;

	QDomNode child = dataManagerNode.firstChild();
	for (; !child.isNull(); child = child.nextSibling())
	{
		if (child.nodeName() == "data")
		{
			DataPtr data = this->loadData(child.toElement(), rootPath);
			if (data)
				datanodes[data] = child.toElement();
		}
	}

	// parse xml data separately: we want to first load all data
	// because there might be interdependencies (cx::DistanceMetric)
	for (std::map<DataPtr, QDomNode>::iterator iter = datanodes.begin(); iter != datanodes.end(); ++iter)
	{
		iter->first->parseXml(iter->second);
	}

	emit dataAddedOrRemoved();

	//we need to make sure all images are loaded before we try to set an active image
	child = dataManagerNode.firstChild();
	while (!child.isNull())
	{
		if (child.toElement().tagName() == "activeImageUid")
		{
			const QString activeImageString = child.toElement().text();
			if (!activeImageString.isEmpty())
			{
				ImagePtr image = this->getImage(activeImageString);
				this->setActiveImage(image);
			}
		}
		//TODO add activeMesh
		if (child.toElement().tagName() == "center")
		{
			const QString centerString = child.toElement().text();
			if (!centerString.isEmpty())
			{
				Vector3D center = Vector3D::fromString(centerString);
				this->setCenter(center);
			}
		}
		child = child.nextSibling();
	}
}

DataPtr DataManagerImpl::loadData(QDomElement node, QString rootPath)
{
	QString uid = node.toElement().attribute("uid");
	QString name = node.toElement().attribute("name");
	QString type = node.toElement().attribute("type");

	QDir relativePath = this->findRelativePath(node, rootPath);
	QString absolutePath = this->findAbsolutePath(relativePath, rootPath);

	if (mData.count(uid)) // dont load same image twice
		return mData[uid];

	DataPtr data = mDataFactory->create(type, uid, name);
	if (!data)
	{
		reportWarning(QString("Unknown type: %1 for file %2").arg(type).arg(absolutePath));
		return DataPtr();
	}
	bool loaded = data->load(absolutePath);

	if (!loaded)
	{
		reportWarning("Unknown file: " + absolutePath);
		return DataPtr();
	}

	if (!name.isEmpty())
		data->setName(name);
	data->setFilename(relativePath.path());

	this->loadData(data);

	// conversion for change in format 2013-10-29
	QString newPath = rootPath+"/"+data->getFilename();
	if (QDir::cleanPath(absolutePath) != QDir::cleanPath(newPath))
	{
		reportWarning(QString("Detected old data format, converting from %1 to %2").arg(absolutePath).arg(newPath));
		data->save(rootPath);
	}

	return data;
}

QDir DataManagerImpl::findRelativePath(QDomElement node, QString rootPath)
{
	QString path = this->findPath(node);
	QDir relativePath = QDir(QString(path));

	QDir patientDataDir(rootPath);
	relativePath.setPath(patientDataDir.relativeFilePath(relativePath.path()));

	return relativePath;
}

QString DataManagerImpl::findPath(QDomElement node)
{
	QDomElement filePathNode = node.namedItem("filePath").toElement();

	if (filePathNode.isNull())
		return QString();

	QString path = filePathNode.text();
	if (path.isEmpty())
		return QString();
	return path;
}

QString DataManagerImpl::findAbsolutePath(QDir relativePath, QString rootPath)
{
	QString absolutePath = relativePath.path();
	if (!rootPath.isEmpty())
		absolutePath = rootPath + "/" + relativePath.path();
	return absolutePath;
}

void DataManagerImpl::vtkImageDataChangedSlot()
{
	QString uid = "";
	if (mActiveImage)
		uid = mActiveImage->getUid();
}

CLINICAL_VIEW DataManagerImpl::getClinicalApplication() const
{
	return mClinicalApplication;
}

void DataManagerImpl::setClinicalApplication(CLINICAL_VIEW application)
{
	if (mClinicalApplication == application)
		return;
	mClinicalApplication = application;

	QString val = enum2string<CLINICAL_VIEW>(mClinicalApplication);
	settings()->setValue("View/clinicalView", val);

	emit clinicalApplicationChanged();
}

void DataManagerImpl::settingsChangedSlot(QString key)
{
	if (key == "View/clinicalView")
	{
		this->readClinicalView();
	}
}

void DataManagerImpl::readClinicalView()
{
	QString defVal = enum2string<CLINICAL_VIEW>(mdNEUROLOGICAL);
	QString val = settings()->value("View/clinicalView", defVal).toString();
	CLINICAL_VIEW view = string2enum<CLINICAL_VIEW>(val);

	this->setClinicalApplication(view);
}


int DataManagerImpl::findUniqueUidNumber(QString uidBase) const
{
	if (!uidBase.contains("%"))
		return -1;
	// Find an uid that is not used before
	size_t numMatches = 1;
	int recNumber = 0;

	if (numMatches != 0)
	{
		while (numMatches != 0)
		{
			QString newId = qstring_cast(uidBase).arg(++recNumber);
			numMatches = mData.count(qstring_cast(newId));
		}
	}
	return recNumber;
}

/**Insert uid and name containing %1 placeholders for insertion of unique integers.
 * Return unique values.
 * If input does not contain %1, nothing happens.
 */
void DataManagerImpl::generateUidAndName(QString* _uid, QString* _name)
{
	QString& uid = *_uid;
	QString& name = *_name;

	int recNumber = this->findUniqueUidNumber(uid);

	if (uid.contains("%"))
	{
		uid = uid.arg(recNumber);
	}

	if (name.contains("%"))
	{
		if (recNumber == 1)
			name = name.arg("");
		else
			name = name.arg(recNumber);
	}
}

void DataManagerImpl::removeData(const QString& uid, QString basePath)
{
	if (this->getActiveImage() && this->getActiveImage()->getUid() == uid)
		this->setActiveImage(ImagePtr());

	DataPtr data = this->getData(uid);

	mData.erase(uid);

	this->deleteFiles(data, basePath);

	emit dataAddedOrRemoved(); // this should alert everybody interested in the data as a collection.
	report("Removed data [" + uid + "].");
}

void DataManagerImpl::deleteFiles(DataPtr data, QString basePath)
{
	if (!data)
		return;
	ImagePtr image = boost::dynamic_pointer_cast<Image>(data);
	QStringList files;
	if (!data->getFilename().isEmpty())
		files << QDir(basePath).absoluteFilePath(data->getFilename());

	if (image)
	{
		files <<  changeExtension(files[0], "raw");
	}

	for (int i=0; i<files.size(); ++i)
	{
		if (!QFileInfo(files[i]).exists())
			continue;
		report(QString("Removing %1 from disk").arg(files[i]));
		QFile(files[i]).remove();
	}
}

Transform3D DataManagerImpl::get_rMpr() const
{
	return m_rMpr_History->getCurrentRegistration().mValue;
}

void DataManagerImpl::set_rMpr(const Transform3D& val)
{
	m_rMpr_History->setRegistration(val);
}

RegistrationHistoryPtr DataManagerImpl::get_rMpr_History() const
{
	return m_rMpr_History;
}

PresetTransferFunctions3DPtr DataManagerImpl::getPresetTransferFunctions3D() const
{
	///< create from filename, create trivial document of type name and root node if no file exists.
	XmlOptionFile preset(DataLocations::findConfigFilePath("presets.xml", "/transferFunctions"));
	XmlOptionFile custom = profile()->getXmlSettings().descend("presetTransferFunctions");

	if (!mPresetTransferFunctions3D)
		mPresetTransferFunctions3D.reset(new TransferFunctions3DPresets(preset, custom));

	return mPresetTransferFunctions3D;
}

} // namespace cx

