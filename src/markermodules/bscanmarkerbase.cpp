#include "bscanmarkerbase.h"


#include <octdata/datastruct/series.h>
#include <octdata/datastruct/bscan.h>

#include <QToolBar>


#include <manager/bscanmarkermanager.h>

std::size_t BscanMarkerBase::getActBScanNr() const
{
	return static_cast<std::size_t>(markerManager->getActBScan());
}

int BscanMarkerBase::getBScanWidth(std::size_t nr) const
{
	const OctData::Series* series = getSeries();
	if(series)
		return series->getBScan(nr)->getWidth();
	return 0;
}

int BscanMarkerBase::getBScanWidth() const
{
	return getBScanWidth(getActBScanNr());
}

int BscanMarkerBase::getBScanHight() const
{
	const OctData::Series* series = getSeries();
	if(series)
		return series->getBScan(getActBScanNr())->getHeight();
	return 0;
}

const OctData::Series* BscanMarkerBase::getSeries() const
{
	return markerManager->getSeries();
}

const OctData::BScan* BscanMarkerBase::getActBScan() const
{
	const OctData::Series* series = getSeries();
	if(series)
		return series->getBScan(getActBScanNr());
	return nullptr;
}

void BscanMarkerBase::activate(bool b)
{
	isActivated = b;
	enabledToolbar(b);
}


void BscanMarkerBase::connectToolBar(QToolBar* toolbar)
{
	connect(this, &BscanMarkerBase::enabledToolbar, toolbar, &QToolBar::setEnabled);
	toolbar->setEnabled(isActivated);
}

