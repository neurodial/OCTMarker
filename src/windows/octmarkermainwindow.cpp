#include "octmarkermainwindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QFileDialog>
#include <QStringList>
#include <QSignalMapper>
#include <QtGui>

#include <QActionGroup>
#include <QSpinBox>
#include <QLabel>

#include <widgets/wgsloimage.h>
#include <widgets/bscanmarkerwidget.h>
#include <widgets/wgoctdatatree.h>
#include <widgets/dwoctinformations.h>
#include <widgets/dwmarkerwidgets.h>
#include <widgets/scrollareapan.h>

#include <data_structure/intervalmarker.h>
#include <data_structure/programoptions.h>

#include <manager/octmarkermanager.h>
#include <manager/octdatamanager.h>
#include <manager/octmarkerio.h>
#include <markermodules/bscanmarkerbase.h>

#include <model/octfilesmodel.h>
#include <model/octdatamodel.h>

#include <octdata/datastruct/sloimage.h>
#include <octdata/datastruct/bscan.h>
#include <octdata/datastruct/series.h>

#include <octdata/octfileread.h>
#include <octdata/filereadoptions.h>

#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <boost/algorithm/string/join.hpp>

#include <cpp_framework/cvmat/treestructbin.h>

#include <globaldefinitions.h>
#include <buildconstants.h>




OCTMarkerMainWindow::OCTMarkerMainWindow(const char* filename)
: QMainWindow()
, markerManager      (new OctMarkerManager      )
, dwSloImage         (new QDockWidget(this))
, bscanMarkerWidget  (new BScanMarkerWidget(*markerManager))
{
	QSettings& settings = ProgramOptions::getSettings();
	restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
	
	
	ScrollAreaPan* bscanMarkerWidgetScrollArea = new ScrollAreaPan(this);
	bscanMarkerWidgetScrollArea->setWidget(bscanMarkerWidget);

	// General Objects
	setCentralWidget(bscanMarkerWidgetScrollArea);

	setupMenu();
	createMarkerToolbar();
	// DockWidgets
	dwSloImage->setObjectName("DWSloImage");
	dwSloImage->setWidget(new WgSloImage(*markerManager, this));
	dwSloImage->setWindowTitle(tr("SLO image"));
	addDockWidget(Qt::LeftDockWidgetArea, dwSloImage);


	WGOctDataTree* tree = new WGOctDataTree();
	QDockWidget* treeDock = new QDockWidget(tr("Oct Data"), this);
	treeDock->setObjectName("DWOctDataTree");
	treeDock->setWidget(tree);
	addDockWidget(Qt::RightDockWidgetArea, treeDock);
	
	DwOctInformations* dwoctinformations = new DwOctInformations(this);
	dwoctinformations->setBScanMarkerManager(markerManager);
	dwoctinformations->setObjectName("DwOctInformations");
	addDockWidget(Qt::RightDockWidgetArea, dwoctinformations);

	DWMarkerWidgets* dwmarkerwidgets = new DWMarkerWidgets(markerManager, this);
	dwmarkerwidgets->setObjectName("DwMarkerWidgets");
	addDockWidget(Qt::LeftDockWidgetArea, dwmarkerwidgets);


	// General Config
	setWindowIcon(QIcon(":/icons/image_edit.png"));
	// setActionToggel();
	setAcceptDrops(true);

	connect(&OctDataManager::getInstance(), &OctDataManager::seriesChanged   , this, &OCTMarkerMainWindow::newCscanLoaded);
	connect(this, &OCTMarkerMainWindow::loadLastFile, this, &OCTMarkerMainWindow::loadLastFileSlot, Qt::QueuedConnection);

	
	for(BscanMarkerBase* marker : markerManager->getBscanMarker())
	{
		QToolBar* toolbar = marker->createToolbar(this);
		if(toolbar)
			addToolBar(toolbar);
	}

	restoreState(settings.value("mainWindowState").toByteArray());

	if(filename)
		loadFile(filename);
	else if(!ProgramOptions::loadOctdataAtStart().isEmpty())
		emit(loadLastFile());
}

void OCTMarkerMainWindow::loadLastFileSlot()
{
	if(!ProgramOptions::loadOctdataAtStart().isEmpty())
		loadFile(ProgramOptions::loadOctdataAtStart());
}


OCTMarkerMainWindow::~OCTMarkerMainWindow()
{
	delete markerManager;
}


namespace
{
	void addMenuProgramOptionGroup(const QString& name, OptionInt& obj, QMenu* menu, SendInt& sendObj, QActionGroup* group, QObject* parent)
	{
		QAction* action = new QAction(parent);
		action->setText(name);
		action->setCheckable(true);
		action->setActionGroup(group);
		sendObj.connectOptions(obj, action);

		menu->addAction(action);
	}

}



void OCTMarkerMainWindow::setupMenu()
{

	// ----------
	// fileMenu
	// ----------
	QMenu* fileMenu = new QMenu(this);
	fileMenu->setTitle(tr("File"));

	QAction* actionLoadImage = new QAction(this);
	actionLoadImage->setText(tr("Load OCT scan"));
	actionLoadImage->setShortcut(Qt::CTRL | Qt::Key_O);
	actionLoadImage->setIcon(QIcon(":/icons/folder.png"));
	connect(actionLoadImage, &QAction::triggered, this, &OCTMarkerMainWindow::showLoadImageDialog);
	fileMenu->addAction(actionLoadImage);

	fileMenu->addSeparator();

	QAction* loadMarkersAction = new QAction(this);
	loadMarkersAction->setText(tr("load markers"));
	loadMarkersAction->setIcon(QIcon(":/icons/folder.png"));
	connect(loadMarkersAction, &QAction::triggered, this, &OCTMarkerMainWindow::showLoadMarkersDialog);
	fileMenu->addAction(loadMarkersAction);

/*
	QAction* addMarkersAction = new QAction(this);
	addMarkersAction->setText(tr("add markers"));
	addMarkersAction->setIcon(QIcon(":/icons/folder_add.png"));
	connect(addMarkersAction, SIGNAL(triggered()), SLOT(showAddMarkersDialog()));
	fileMenu->addAction(addMarkersAction);
*/

	QAction* saveMarkersAction = new QAction(this);
	saveMarkersAction->setText(tr("save markers"));
	saveMarkersAction->setIcon(QIcon(":/icons/disk.png"));
	connect(saveMarkersAction, &QAction::triggered, this, &OCTMarkerMainWindow::showSaveMarkersDialog);
	fileMenu->addAction(saveMarkersAction);

	QAction* saveMarkersDefaultAction = new QAction(this);
	saveMarkersDefaultAction->setText(tr("trigger auto save markers"));
	saveMarkersDefaultAction->setIcon(QIcon(":/icons/disk.png"));
	saveMarkersDefaultAction->setShortcut(Qt::CTRL | Qt::Key_S);
	connect(saveMarkersDefaultAction, &QAction::triggered, &(OctDataManager::getInstance()), &OctDataManager::triggerSaveMarkersDefault);
	fileMenu->addAction(saveMarkersDefaultAction);


	fileMenu->addSeparator();

	QAction* actionQuit = new QAction(this);
	actionQuit->setText(tr("Quit"));
	actionQuit->setIcon(QIcon(":/icons/door_in.png"));
	connect(actionQuit, &QAction::triggered, this, &OCTMarkerMainWindow::close);
	fileMenu->addAction(actionQuit);


	// ----------
	// Options
	// ----------
	QMenu* optionsMenu = new QMenu(this);
	optionsMenu->setTitle(tr("Options"));

	QAction* fillEpmtyPixelWhite = ProgramOptions::fillEmptyPixelWhite.getAction();
	QAction* registerBScans      = ProgramOptions::registerBScans     .getAction();
	QAction* loadRotateSlo       = ProgramOptions::loadRotateSlo      .getAction();
	QAction* holdOCTRawData      = ProgramOptions::holdOCTRawData     .getAction();
	fillEpmtyPixelWhite->setText(tr("Fill empty pixels white"));
	registerBScans     ->setText(tr("register BScans"));
	loadRotateSlo      ->setText(tr("rotate SLO"));
	holdOCTRawData     ->setText(tr("hold OCT raw data"));
	optionsMenu->addAction(fillEpmtyPixelWhite);
	optionsMenu->addAction(registerBScans);
	optionsMenu->addAction(loadRotateSlo );
	optionsMenu->addAction(holdOCTRawData);

	QMenu* optionsMenuE2E = new QMenu(this);
	optionsMenuE2E->setTitle(tr("E2E Gray"));
	optionsMenu->addMenu(optionsMenuE2E);

	QActionGroup* e2eGrayTransformGroup = new QActionGroup(this);
	static SendInt e2eGrayNativ(static_cast<int>(OctData::FileReadOptions::E2eGrayTransform::nativ));
	addMenuProgramOptionGroup(tr("E2E nativ"    ), ProgramOptions::e2eGrayTransform, optionsMenuE2E, e2eGrayNativ, e2eGrayTransformGroup, this);
	static SendInt e2eGrayXml   (static_cast<int>(OctData::FileReadOptions::E2eGrayTransform::xml));
	addMenuProgramOptionGroup(tr("Xml emulation"), ProgramOptions::e2eGrayTransform, optionsMenuE2E, e2eGrayXml  , e2eGrayTransformGroup, this);
	static SendInt e2eGrayVol   (static_cast<int>(OctData::FileReadOptions::E2eGrayTransform::vol));
	addMenuProgramOptionGroup(tr("Vol emulation"), ProgramOptions::e2eGrayTransform, optionsMenuE2E, e2eGrayVol  , e2eGrayTransformGroup, this);

	optionsMenu->addSeparator();



	QAction* autoSaveOctMarkers       = ProgramOptions::autoSaveOctMarkers.getAction();
	autoSaveOctMarkers->setText(tr("Autosave markers"));
	optionsMenu->addAction(autoSaveOctMarkers);


	QMenu* optionsMenuMarkersFileFormat = new QMenu(this);
	optionsMenuMarkersFileFormat->setTitle(tr("Default filetype"));
	optionsMenu->addMenu(optionsMenuMarkersFileFormat);

	QActionGroup* markersFileFormatGroup = new QActionGroup(this);
	static SendInt markerFFxml(OctMarkerIO::fileformat2Int(OctMarkerFileformat::XML));
	addMenuProgramOptionGroup(tr("XML" ), ProgramOptions::defaultFileformatOctMarkers, optionsMenuMarkersFileFormat, markerFFxml, markersFileFormatGroup, this);
	static SendInt markerFFjsom(OctMarkerIO::fileformat2Int(OctMarkerFileformat::Json));
	addMenuProgramOptionGroup(tr("JSOM"), ProgramOptions::defaultFileformatOctMarkers, optionsMenuMarkersFileFormat, markerFFjsom  , markersFileFormatGroup, this);
	static SendInt markerFFinfo(OctMarkerIO::fileformat2Int(OctMarkerFileformat::INFO));
	addMenuProgramOptionGroup(tr("INFO"), ProgramOptions::defaultFileformatOctMarkers, optionsMenuMarkersFileFormat, markerFFinfo  , markersFileFormatGroup, this);

	// ----------
	// Extras
	// ----------

	QMenu* extrisMenu = new QMenu(this);
	extrisMenu->setTitle(tr("Extras"));

	QAction* actionSaveMatlabBinCode = new QAction(this);
	actionSaveMatlabBinCode->setText(tr("Save Matlab Bin Code"));
	actionSaveMatlabBinCode->setIcon(QIcon(":/icons/disk.png"));
	connect(actionSaveMatlabBinCode, &QAction::triggered, this, &OCTMarkerMainWindow::saveMatlabBinCode);
	extrisMenu->addAction(actionSaveMatlabBinCode);

	QAction* actionSaveMatlabWriteBinCode = new QAction(this);
	actionSaveMatlabWriteBinCode->setText(tr("Save Matlab Write Bin Code"));
	actionSaveMatlabWriteBinCode->setIcon(QIcon(":/icons/disk.png"));
	connect(actionSaveMatlabWriteBinCode, &QAction::triggered, this, &OCTMarkerMainWindow::saveMatlabWriteBinCode);
	extrisMenu->addAction(actionSaveMatlabWriteBinCode);

	// ----------
	// Help Menu
	// ----------

	QMenu* helpMenu = new QMenu(this);
	helpMenu->setTitle(tr("Help"));

	QAction* aboutQtAct = new QAction(this);
	aboutQtAct->setText(tr("About Qt"));
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
	aboutQtAct->setIcon(QIcon(":/icons/help.png"));
	connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	helpMenu->addAction(aboutQtAct);

	QAction* actionAboutDialog = new QAction(this);
	actionAboutDialog->setText(tr("About"));
	actionAboutDialog->setIcon(QIcon(":/icons/image_edit.png"));
	connect(actionAboutDialog, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
	helpMenu->addAction(actionAboutDialog);



	menuBar()->addMenu(fileMenu);
	menuBar()->addMenu(optionsMenu);
	menuBar()->addMenu(extrisMenu);
	menuBar()->addMenu(helpMenu);


	//
	// Toolbar
	//

	QAction* previousOctScan = new QAction(this);
	previousOctScan->setText(tr("previous octScan"));
	previousOctScan->setIcon(QIcon(":/icons/resultset_previous.png"));
	previousOctScan->setShortcut(Qt::CTRL + Qt::LeftArrow);
	connect(previousOctScan, &QAction::triggered, &(OctFilesModel::getInstance()), &OctFilesModel::loadPreviousFile);

	QAction* nextOctScan = new QAction(this);
	nextOctScan->setText(tr("next octScan"));
	nextOctScan->setIcon(QIcon(":/icons/resultset_next.png"));
	nextOctScan->setShortcut(Qt::CTRL + Qt::RightArrow);
	connect(nextOctScan, &QAction::triggered, &(OctFilesModel::getInstance()), &OctFilesModel::loadNextFile);



	QAction* nextBScan = new QAction(this);
	nextBScan->setText(tr("next bscan"));
	nextBScan->setIcon(QIcon(":/icons/arrow_right.png"));
	nextBScan->setShortcut(Qt::RightArrow);
	connect(nextBScan, SIGNAL(triggered()), markerManager, SLOT(nextBScan()));

	bscanChooser = new QSpinBox(this);
	connect(bscanChooser, SIGNAL(valueChanged(int)), markerManager, SLOT(chooseBScan(int)));
	connect(markerManager, SIGNAL(bscanChanged(int)), bscanChooser, SLOT(setValue(int)));

	labelMaxBscan = new QLabel(this);
	labelMaxBscan->setText("/0");


	QAction* previousBScan = new QAction(this);
	previousBScan->setText(tr("previous bscan"));
	previousBScan->setIcon(QIcon(":/icons/arrow_left.png"));
	previousBScan->setShortcut(Qt::LeftArrow);
	connect(previousBScan, SIGNAL(triggered(bool)), markerManager, SLOT(previousBScan()));

	QAction* showSeglines = ProgramOptions::bscansShowSegmentationslines.getAction();
	showSeglines->setText(tr("show segmentationslines"));
	showSeglines->setIcon(QIcon(":/icons/chart_curve.png"));

	QToolBar* toolBar = new QToolBar(tr("B-Scan"));
	toolBar->setObjectName("ToolBarBScan");
	toolBar->addAction(previousOctScan);
	toolBar->addAction(nextOctScan);
	toolBar->addSeparator();
	toolBar->addAction(previousBScan);
	toolBar->addAction(nextBScan);
	toolBar->addSeparator();
	toolBar->addWidget(bscanChooser);
	toolBar->addWidget(labelMaxBscan);
	toolBar->addSeparator();
	toolBar->addAction(showSeglines);





	toolBar->addSeparator();
	QAction* zoomIn = new QAction(this);
	zoomIn->setText(tr("Zoom +"));
	zoomIn->setIcon(QIcon(":/icons/zoom_in.png"));
	connect(zoomIn, &QAction::triggered, bscanMarkerWidget, &CVImageWidget::zoom_in);
	toolBar->addAction(zoomIn);

	labelActZoom = new QLabel(this);
	labelActZoom->setText("1");
	toolBar->addWidget(labelActZoom);
	connect(bscanMarkerWidget, &CVImageWidget::zoomChanged, this, &OCTMarkerMainWindow::zoomChanged);


	QAction* zoomOut = new QAction(this);
	zoomOut->setText(tr("Zoom -"));
	zoomOut->setIcon(QIcon(":/icons/zoom_out.png"));
	connect(zoomOut, &QAction::triggered, bscanMarkerWidget, &CVImageWidget::zoom_out);
	toolBar->addAction(zoomOut);



	addToolBar(toolBar);
}

void OCTMarkerMainWindow::zoomChanged(double zoom)
{
	labelActZoom->setText(QString("%1").arg(zoom));
}


void OCTMarkerMainWindow::createMarkerToolbar()
{
	const std::vector<BscanMarkerBase*>& markers = markerManager->getBscanMarker();
	
	QToolBar*      toolBar            = new QToolBar(tr("Marker"));
	QActionGroup*  actionGroupMarker  = new QActionGroup(this);
	QSignalMapper* signalMapperMarker = new QSignalMapper(this);
	
	toolBar->setObjectName("ToolBarMarkerChoose");

	QAction* markerAction = new QAction(this);
	markerAction->setCheckable(true);
	markerAction->setText(tr("no marker"));
	markerAction->setIcon(QIcon(":/icons/image.png"));
	markerAction->setChecked(markerManager->getActBscanMarkerId() == -1);
	connect(markerAction, &QAction::triggered, signalMapperMarker, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
	signalMapperMarker->setMapping(markerAction, -1);
	actionGroupMarker->addAction(markerAction);
	toolBar->addAction(markerAction);
	
	int id = 0;
	for(BscanMarkerBase* marker : markers)
	{
		QAction* markerAction = new QAction(this);
		markerAction->setCheckable(true);
		markerAction->setText(marker->getName());
		markerAction->setIcon(marker->getIcon());
		markerAction->setChecked(markerManager->getActBscanMarkerId() == id);
		connect(markerAction, &QAction::triggered, signalMapperMarker, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
		signalMapperMarker->setMapping(markerAction, id);

		actionGroupMarker->addAction(markerAction);
		toolBar->addAction(markerAction);
		++id;
	}
	connect(signalMapperMarker, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), markerManager, &OctMarkerManager::setBscanMarker);
	
	addToolBar(toolBar);
}



void OCTMarkerMainWindow::showAboutDialog()
{
	QString text("<h1><b>Info &#252;ber OCT-Marker</b></h1>");

	// text += "<br />Dieses Programm entstand im Rahmen der Masterarbeit &#8222;Aktive-Konturen-Methoden zur Segmentierung von OCT-Aufnahmen&#8220;";
	text += "<br /><br />Author: Kay Gawlik";
	text += "<br /><br />Icons von <a href=\"http://www.famfamfam.com/lab/icons/silk/\">FamFamFam</a>";

	text += "<br/><br/><b>Build-Informationen</b><table>";
	text += QString("<tr><td>&#220;bersetzungszeit </td><td> %1 %2</td></tr>").arg(BuildConstants::buildDate).arg(BuildConstants::buildTime);
	text += QString("<tr><td>Qt-Version </td><td> %1</td></tr>").arg(qVersion());
	text += QString("<tr><td>Git-Hash   </td><td> %1</td></tr>").arg(BuildConstants::gitSha1);
	text += QString("<tr><td>Build-Typ  </td><td> %1</td></tr>").arg(BuildConstants::buildTyp);
	text += QString("<tr><td>Compiler   </td><td> %1 %2</td></tr>").arg(BuildConstants::compilerId).arg(BuildConstants::compilerVersion);
	text += "</table>";

	QMessageBox::about(this,tr("About"), text);
}


void OCTMarkerMainWindow::showLoadImageDialog()
{
	QFileDialog fd;
	// fd.selectFile(lineEditFile->text());
	fd.setWindowTitle(tr("Choose a filename to load a File"));
	fd.setAcceptMode(QFileDialog::AcceptOpen);


	QStringList allExtentions;
	QStringList filtersOct;

	for(const OctData::OctExtension& ext : OctData::OctFileRead::supportedExtensions())
	{
		QStringList extensions;
		for(const std::string& str : ext.extensions)
		{
			if(!str.empty())
			{
				if(str[0] == '.')
				{
					extensions << QString("*%1").arg(str.c_str());
					allExtentions << QString("*%1").arg(str.c_str());
				}
				else
				{
					extensions << str.c_str();
					allExtentions << str.c_str();
				}
			}
		}
		// std::string extensions = boost::algorithm::join(ext.extensions, " ");
		filtersOct << QString("%1 (%2)").arg(ext.name.c_str()).arg(extensions.join(' '));
	}

	filtersOct.sort();
	
	QStringList filtersAllFiles;
	filtersAllFiles << tr("All readabel (%1)").arg(allExtentions.join(' '));
	filtersAllFiles << tr("All files (* *.*)");

	QStringList filters = filtersAllFiles + filtersOct;



	fd.setNameFilters(filters);
	fd.setFileMode(QFileDialog::ExistingFile);

	// fd.setDirectory("~/oct/");

	if(fd.exec())
	{
		QStringList filenames = fd.selectedFiles();
		loadFile(filenames[0]);
	}
}


void OCTMarkerMainWindow::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();

	// check for our needed mime type, here a file or a list of files
	if(mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();
		if(urlList.size() == 1)
		{
			loadFile(urlList.at(0).toLocalFile());
		}
		else
		{
			for(const QUrl& url : urlList)
			{
				if(OctData::OctFileRead::isLoadable(url.toLocalFile().toStdString()))
				{
					addFile(url.toLocalFile());
				}
			}
		}
	}
}

void OCTMarkerMainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if(mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();
		for(const QUrl& url : urlList)
		{
			if(OctData::OctFileRead::isLoadable(url.toLocalFile().toStdString()))
			{
				event->acceptProposedAction();
				break;
			}
		}
	}
}

void OCTMarkerMainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
	QWidget::dragLeaveEvent(event);
}

void OCTMarkerMainWindow::dragMoveEvent(QDragMoveEvent* event)
{
	QWidget::dragMoveEvent(event);
}


bool OCTMarkerMainWindow::loadFile(const QString& filename)
{
	return OctFilesModel::getInstance().loadFile(filename);
}

bool OCTMarkerMainWindow::addFile(const QString& filename)
{
	return OctFilesModel::getInstance().addFile(filename);
}

// bool OCTMarkerMainWindow::loadFolder(const QString& foldername)
// {
// 	return OctFilesModel::getInstance().addFile(filename);
// }
//






/*

void OCTMarkerMainWindow::showAddMarkersDialog()
{
	QFileDialog fd;
	fd.setWindowTitle(tr("Choose a file to add markers"));
	fd.setAcceptMode(QFileDialog::AcceptOpen);

	fd.setNameFilter(tr("OCT Markers file (*_markers.xml)"));
	fd.setFileMode(QFileDialog::ExistingFile);

	if(fd.exec())
	{
		QStringList filenames = fd.selectedFiles();
		OctDataManager::getInstance().addMarkers(filenames[0], OctDataManager::Fileformat::XML);
	}
}
*/

void OCTMarkerMainWindow::setMarkersStringList(QStringList& filters)
{
	const char* josnExt = OctMarkerIO::getFileExtension(OctMarkerFileformat::Json);
	const char*  xmlExt = OctMarkerIO::getFileExtension(OctMarkerFileformat::XML);
	const char* infoExt = OctMarkerIO::getFileExtension(OctMarkerFileformat::INFO);
	
	filters << tr("OCT Markers")+QString(" (*.%1 *.%2 *.%3)").arg(josnExt).arg(xmlExt).arg(infoExt);
	filters << tr("OCT Markers Json file")+QString(" (*.%1)").arg(josnExt);
	filters << tr("OCT Markers XML file" )+QString(" (*.%1)").arg(xmlExt);
	filters << tr("OCT Markers INFO file")+QString(" (*.%1)").arg(infoExt);
}

namespace
{
	OctMarkerFileformat getMarkerFileFormat(const QString& filter)
	{
		QStringList filters;
		OCTMarkerMainWindow::setMarkersStringList(filters);
		int index = filters.indexOf(filter);
		
		static const OctMarkerFileformat formats[] = {OctMarkerFileformat::Json, OctMarkerFileformat::XML, OctMarkerFileformat::INFO};
		
		if(index == 0)
		   return OctMarkerFileformat::Auto;
		
		--index;
		if(index < static_cast<int>(sizeof(formats)/sizeof(formats[0])))
		{
			return formats[index];
		}
		
		throw "unknown filtername";
	}
}


void OCTMarkerMainWindow::setMarkersFilters(QFileDialog& fd)
{
	QStringList filters;
	setMarkersStringList(filters);
	fd.setNameFilters(filters);
}


void OCTMarkerMainWindow::showLoadMarkersDialog()
{
	QFileDialog fd;
	fd.setWindowTitle(tr("Choose a file to load markers"));
	fd.setAcceptMode(QFileDialog::AcceptOpen);

	setMarkersFilters(fd);
	fd.setFileMode(QFileDialog::ExistingFile);

	if(fd.exec())
	{
		QStringList filenames = fd.selectedFiles();
		OctDataManager::getInstance().loadMarkers(filenames[0], getMarkerFileFormat(fd.selectedNameFilter()));
	}
}

void OCTMarkerMainWindow::showSaveMarkersDialog()
{
	QFileDialog fd;
	fd.setWindowTitle(tr("Choose a filename to save markers"));
	fd.setAcceptMode(QFileDialog::AcceptSave);

	setMarkersFilters(fd);
	fd.setFileMode(QFileDialog::AnyFile);

	if(fd.exec())
	{
		QStringList filenames = fd.selectedFiles();
		OctDataManager::getInstance().saveMarkers(filenames[0], getMarkerFileFormat(fd.selectedNameFilter()));
	}
}

void OCTMarkerMainWindow::newCscanLoaded()
{
	setWindowTitle(tr("OCT-Marker - %1").arg(OctDataManager::getInstance().getLoadedFilename()));
	configBscanChooser();
}

void OCTMarkerMainWindow::configBscanChooser()
{
	const OctData::Series* series = OctDataManager::getInstance().getSeries();
	
	std::size_t maxBscan = 0;
	if(series)
	{
		maxBscan = series->bscanCount();
		if(maxBscan > 0)
			--maxBscan;
	}
	
	bscanChooser->setMaximum(static_cast<int>(maxBscan));
	bscanChooser->setValue(markerManager->getActBScan());

	labelMaxBscan->setText(QString("/%1").arg(maxBscan));
}

void OCTMarkerMainWindow::saveMatlabBinCode()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save Matlab Bin Code"), QString("readbin.m"), "Matlab (*.m)");
	if(!filename.isEmpty())
		CppFW::CVMatTreeStructBin::writeMatlabReadCode(filename.toStdString().c_str());
}

void OCTMarkerMainWindow::saveMatlabWriteBinCode()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save Matlab Write Bin Code"), QString("writebin.m"), "Matlab (*.m)");
	if(!filename.isEmpty())
		CppFW::CVMatTreeStructBin::writeMatlabWriteCode(filename.toStdString().c_str());
}

void OCTMarkerMainWindow::closeEvent(QCloseEvent* e)
{
	// save Markers
	bool saveSuccessful = true;
	std::string errorStr;
	try
	{
		OctDataManager::getInstance().saveMarkersDefault();
	}
	catch(boost::exception& e)
	{
		saveSuccessful = false;
		errorStr = boost::diagnostic_information(e);
	}
	catch(std::exception& e)
	{
		saveSuccessful = false;
		errorStr = e.what();
	}
	catch(const char* str)
	{
		saveSuccessful = false;
		errorStr = str;
	}
	catch(...)
	{
		saveSuccessful = false;
		errorStr = tr("Unknown error on autosave").toStdString();
	}
	if(!saveSuccessful)
	{
		int ret = QMessageBox::critical(this, tr("Error on autosave"), tr("Autosave fail with message: %1 <br />Quit program?").arg(errorStr.c_str()), QMessageBox::Yes | QMessageBox::No);
		if(ret == QMessageBox::No)
			return e->ignore();
	}



	// save programoptions
	ProgramOptions::loadOctdataAtStart.setValue(OctDataManager::getInstance().getLoadedFilename());
	
	ProgramOptions::writeAllOptions();

	QSettings& settings = ProgramOptions::getSettings();
	

	settings.setValue("mainWindowGeometry", saveGeometry());
	settings.setValue("mainWindowState"   , saveState()   );

	QWidget::closeEvent(e);
}


void SendInt::connectOptions(OptionInt& option, QAction* action)
{
	connect(action, &QAction::toggled, this   , &SendInt::recive    );
	connect(this  , &SendInt::send   , &option, &OptionInt::setValue);

	action->setChecked(option() == v);
}
