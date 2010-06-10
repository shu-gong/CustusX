#include "probeXmlConfigParser.h"

#include <iostream>
#include <QFile>
#include <QStringList>

ProbeXmlConfigParser::ProbeXmlConfigParser(QString& pathToXml)
{
  QFile file(pathToXml);
  if(file.open(QIODevice::ReadOnly))
  {
    QString emsg;
    int eline, ecolumn;
    if (!mDomDoc.setContent(&file, false, &emsg, &eline, &ecolumn))
    {
      std::cout << "Could not parse XML file :";
      std::cout << file.fileName().toStdString() << " because: ";
      std::cout << emsg.toStdString() << std::endl;
      throw "Could not parse XML file :";
    }
    file.close();
  }
  else
  {
    std::cout << "Could not open XML file :";
    std::cout << file.fileName().toStdString() << std::endl;
  }
  
}

ProbeXmlConfigParser::~ProbeXmlConfigParser()
{}

QStringList ProbeXmlConfigParser::getScannerList()
{
  QStringList retval;
  QList<QDomNode> scannerNodes = this->getScannerNodes();
  QDomNode node;
  foreach(node, scannerNodes)
  {
    retval << node.firstChildElement("Name").text();
  }
  return retval;
}

QStringList ProbeXmlConfigParser::getProbeList(QString scanner)
{
  QStringList retval;
  QList<QDomNode> probeNodes = this->getProbeNodes(scanner);
  QDomNode node;
  foreach(node, probeNodes)
  {
      retval << node.firstChildElement("Name").text();
  }
  return retval;
}

QStringList ProbeXmlConfigParser::getRtSourceList(QString scanner, QString probe)
{
  QStringList retval;
  QList<QDomNode> rtSourceNodes = this->getRTSourceNodes(scanner, probe);
  QDomNode node;
  foreach(node, rtSourceNodes)
  {
      retval << node.firstChildElement("Name").text();
  }
  return retval;
}

QStringList ProbeXmlConfigParser::getConfigIdList(QString scanner, QString probe, QString rtSource)
{
  QStringList retval;
  QList<QDomNode> configNodes = this->getConfigNodes(scanner, probe, rtSource);
  QDomNode node;
  foreach(node, configNodes)
  {
    retval << node.firstChildElement("ID").text();
  }
  return retval;
}

ProbeXmlConfigParser::Configuration ProbeXmlConfigParser::getConfiguration(QString scanner, QString probe, QString rtsource, QString configId)
{
  Configuration retval;
  
  retval.mUsScanner = scanner.toStdString();
  retval.mUsProbe = probe.toStdString();
  retval.mRtSource = rtsource.toStdString();
  retval.mConfigId = configId.toStdString();

  QList<QDomNode> currentConfigNodeList = this->getConfigNodes(scanner, probe, rtsource, configId);
  if(currentConfigNodeList.isEmpty())
  {
    std::cout << "getCurrentConfiguration found an empty list for: ";
    std::cout << scanner.toStdString() << " " << probe.toStdString() << " ";
    std::cout << rtsource.toStdString() << " " <<configId.toStdString() << std::endl;
    
    return retval;
  }
  QDomNode configNode = currentConfigNodeList.first();

  try
  {
    QDomElement element = configNode.namedItem("WidthDeg").toElement();
    if(element.isNull())
      throw "Can't find WidthDeg";
    bool ok;
    retval.mWidthDeg = element.text().toInt(&ok);
    if(!ok)
      throw "WidthDeg not a number";

    element = configNode.namedItem("Depth").toElement();
    if(element.isNull())
      throw "Can't find Depth";
    retval.mDepth = element.text().toInt(&ok);
    if(!ok)
      throw "Depth not a number";

    element = configNode.namedItem("Offset").toElement();
    if(element.isNull())
      throw "Can't find Offset";
    retval.mOffset = element.text().toInt(&ok);
    if(!ok)
      throw "Offset not a number";

    QDomNode originNode = configNode.namedItem("Origin");
    element = originNode.namedItem("Col").toElement();
    if(element.isNull())
      throw "Can't find Origin.Col";
    retval.mOriginCol = element.text().toFloat(&ok);
    if(!ok)
      throw "Origin.Col not a number";

    element = originNode.namedItem("Row").toElement();
    if(element.isNull())
      throw "Can't find Origin.Row";
    retval.mOriginRow = element.text().toFloat(&ok);
    if(!ok)
      throw "Origin.Row not a number";

    element = configNode.namedItem("NCorners").toElement();
    if(element.isNull())
      throw "Can't find NCorners";
    retval.mNCorners = element.text().toInt(&ok);
    if(!ok)
      throw "NCorners not a number";

    QDomNode cornerNode = configNode.firstChildElement("Corner");
    if(cornerNode.isNull())
      throw "Can't find Corner";
    retval.mCorners.clear();
    for(int i=0; i<retval.mNCorners ; ++i)
    {
      element = cornerNode.namedItem("Col").toElement();
      if(element.isNull())
        throw "Can't find Corner.Col";
      int col = element.text().toInt(&ok);
      if(!ok)
        throw "Corner.Col not a number";

      element = cornerNode.namedItem("Row").toElement();
      if(element.isNull())
        throw "Can't find Corner.Row";
      int row = element.text().toInt(&ok);
      if(!ok)
        throw "Corner.Row not a number";

      retval.mCorners.push_back(ColRowPair(col,row));

      cornerNode = cornerNode.nextSibling();
    }

    QDomNode edgesNode = configNode.namedItem("Edges");
    element = edgesNode.namedItem("Left").toElement();
    if(element.isNull())
      throw "Can't find Left";
    retval.mLeftEdge = element.text().toInt(&ok);
    if(!ok)
      throw "Left not a number";

    element = edgesNode.namedItem("Right").toElement();
    if(element.isNull())
      throw "Can't find Right";
    retval.mRightEdge = element.text().toInt(&ok);
    if(!ok)
      throw "Right not a number";

    element = edgesNode.namedItem("Top").toElement();
    if(element.isNull())
      throw "Can't find Top";
    retval.mTopEdge = element.text().toInt(&ok);
    if(!ok)
      throw "Top not a number";

    element = edgesNode.namedItem("Bottom").toElement();
    if(element.isNull())
      throw "Can't find Bottom";
    retval.mBottomEdge = element.text().toInt(&ok);
    if(!ok)
      throw "Bottom not a number";

    QDomNode pixelSizeNode = configNode.namedItem("PixelSize");
    element = pixelSizeNode.namedItem("Width").toElement();
    if(element.isNull())
      throw "Can't find PixelSize.Width";
    retval.mPixelWidth = element.text().toDouble(&ok);
    if(!ok)
      throw "PixelSize.Width not a number";

    element = pixelSizeNode.namedItem("Height").toElement();
    if(element.isNull())
      throw "Can't find PixelSize.Height";
    retval.mPixelHeight = element.text().toDouble(&ok);
    if(!ok)
      throw "PixelSize.Height not a number";

  }
  catch( char * str ) {
     std::cout << "Exception raised: " << str << std::endl;
  }
  return retval;
}

QList<QDomNode> ProbeXmlConfigParser::getScannerNodes(QString scanner)
{
  QList<QDomNode> retval;
  if(scanner == "ALL")
  {
    retval = this->nodeListToListOfNodes(mDomDoc.elementsByTagName("USScanner"));
  }
  else
  {
    QList<QDomNode> temp = this->nodeListToListOfNodes(mDomDoc.elementsByTagName("USScanner"));
    QDomNode node;
    foreach(node, temp)
    {
      if(node.firstChildElement("Name").text() == scanner)
       retval.append(node);
    }
  }
  return retval;
}

QList<QDomNode> ProbeXmlConfigParser::getProbeNodes(QString scanner, QString probe)
{
  QList<QDomNode> retval;
  if(probe == "ALL")
  {
    QList<QDomNode> temp = this->getScannerNodes(scanner);
    QDomNode scannerNode;
    foreach(scannerNode, temp)
    {
      retval.append(this->nodeListToListOfNodes(scannerNode.toElement().elementsByTagName("USProbe")));
    }
  }
  else
  {
    QList<QDomNode> temp = this->getScannerNodes(scanner);
    QDomNode scannerNode;
    foreach(scannerNode, temp)
    {
      QList<QDomNode> temp2 = this->nodeListToListOfNodes(scannerNode.toElement().elementsByTagName("USProbe"));
      QDomNode probeNode;
      foreach(probeNode, temp2)
      {
        if(probeNode.firstChildElement("Name").text() == probe)
          retval.append(probeNode);
      }
    }
  }
  return retval;
}

QList<QDomNode> ProbeXmlConfigParser::getRTSourceNodes(QString scanner, QString probe, QString rtSource)
{
  QList<QDomNode> retval;
  if(rtSource == "ALL")
  {
    QList<QDomNode> temp = this->getProbeNodes(scanner, probe);
    QDomNode probeNode;
    foreach(probeNode, temp)
    {
      retval.append(this->nodeListToListOfNodes(probeNode.toElement().elementsByTagName("RTSource")));
    }
  }
  else
  {
    QList<QDomNode> temp = this->getProbeNodes(scanner, probe);
    QDomNode probeNode;
    foreach(probeNode, temp)
    {
      QList<QDomNode> temp2 = this->nodeListToListOfNodes(probeNode.toElement().elementsByTagName("RTSource"));
      QDomNode rtSourceNode;
      foreach(rtSourceNode, temp2)
      {
        if(rtSourceNode.firstChildElement("Name").text() == rtSource)
          retval.append(rtSourceNode);
      }
    }
  }
  return retval;
}

QList<QDomNode> ProbeXmlConfigParser::getConfigNodes(QString scanner, QString probe, QString rtsource, QString config)
{
  QList<QDomNode> retval;
  if(config == "ALL")
  {
    QList<QDomNode> temp = this->getRTSourceNodes(scanner, probe, rtsource);
    QDomNode rtSourceNode;
    foreach(rtSourceNode, temp)
    {
      retval.append(this->nodeListToListOfNodes(rtSourceNode.toElement().elementsByTagName("Config")));
    }
  }
  else
  {
    QList<QDomNode> temp = this->getRTSourceNodes(scanner, probe, rtsource);
    QDomNode rtSourceNode;
    foreach(rtSourceNode, temp)
    {
      QList<QDomNode> temp2 = this->nodeListToListOfNodes(rtSourceNode.toElement().elementsByTagName("Config"));
      QDomNode configNode;
      foreach(configNode, temp2)
      {
        if(configNode.firstChildElement("ID").text() == config)
          retval.append(configNode);
      }
    }
  }
  return retval;
}

QList<QDomNode> ProbeXmlConfigParser::nodeListToListOfNodes(QDomNodeList list)
{
    QList<QDomNode> retval;
    for(int i=0; i < list.count(); ++i)
    {
      retval.append(list.item(i));
    }
    return retval;
}
