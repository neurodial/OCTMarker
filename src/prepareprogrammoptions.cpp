#include"prepareprogrammoptions.h"

#include<data_structure/programoptions.h>


void PrepareProgrammOptions::prepareProgrammOptions()
{

	QAction* layerSegBlendThicknessmapAction = ProgramOptions::layerSegThicknessmapBlend.getAction();
	layerSegBlendThicknessmapAction->setText(tr("Blend Thicknessmap Color"));
	layerSegBlendThicknessmapAction->setIcon(QIcon(":/icons/typicons/waves-outline.svg"));


	QAction* actionShowOverlay = ProgramOptions::sloShowOverlay.getAction();
	actionShowOverlay->setText(tr("show overlay"));
	actionShowOverlay->setIcon(QIcon(":/icons/typicons/brush.svg"));


	QAction* autoSaveOctMarkers = ProgramOptions::autoSaveOctMarkers.getAction();
	autoSaveOctMarkers->setText(tr("Autosave markers"));


	QAction* fillEpmtyPixelWhite = ProgramOptions::fillEmptyPixelWhite.getAction();
	QAction* registerBScans      = ProgramOptions::registerBScans     .getAction();
	QAction* loadRotateSlo       = ProgramOptions::loadRotateSlo      .getAction();
	QAction* holdOCTRawData      = ProgramOptions::holdOCTRawData     .getAction();
	QAction* readBScans          = ProgramOptions::readBScans         .getAction();
	QAction* saveOctBinFlat      = ProgramOptions::saveOctBinFlat     .getAction();
	fillEpmtyPixelWhite->setText(tr("Fill empty pixels white"));
	registerBScans     ->setText(tr("register BScans"));
	loadRotateSlo      ->setText(tr("rotate SLO"));
	holdOCTRawData     ->setText(tr("hold OCT raw data"));
	readBScans         ->setText(tr("read BScans from OCT data"));
	saveOctBinFlat     ->setText(tr("save in octbin flat format"));

	QAction* bscanAutoFitImage = ProgramOptions::bscanAutoFitImage.getAction();
	bscanAutoFitImage->setText(tr("B-scan auto fit"));
	bscanAutoFitImage->setIcon(QIcon::fromTheme("zoom-fit-best",  QIcon(":/icons/tango/actions/view-fullscreen.svgz")));;

	QAction* extraSegLine = ProgramOptions::bscanShowExtraSegmentationslines.getAction();
	extraSegLine->setText(tr("show extra segmentationslines"));
	extraSegLine->setIcon(QIcon(":/icons/chart_curve.png"));

	QAction* showSeglines = ProgramOptions::bscansShowSegmentationslines.getAction();
	showSeglines->setText(tr("show segmentationslines"));
	showSeglines->setIcon(QIcon(":/icons/chart_curve.png"));

	QAction* showGrid = ProgramOptions::sloShowGrid.getAction();
	showGrid->setText(tr("show grid"));
	showGrid->setIcon(QIcon(":/icons/grid.png"));

	QAction* showBScanMousePos = ProgramOptions::sloShowBScanMousePos.getAction();
	showBScanMousePos->setText(tr("show mouse pos"));
	showBScanMousePos->setIcon(QIcon(":/icons/typicons/times-outline.svg"));

	QAction* sloClipScanArea = ProgramOptions::sloClipScanArea.getAction();
	sloClipScanArea->setText(tr("clip to scan area"));
	sloClipScanArea->setIcon(QIcon::fromTheme("view-fullscreen", QIcon(":/icons/tango/actions/system-search.svgz")));

	QAction* intervallMarkSloMapAuteGenerate = ProgramOptions::intervallMarkSloMapAuteGenerate.getAction();
	intervallMarkSloMapAuteGenerate->setText(tr("Generate SLO map"));
	intervallMarkSloMapAuteGenerate->setIcon(QIcon(":/icons/map.png"));


	ProgramOptions::layerSegActiveLineColor.getColorDialogAction()->setText(tr("Active line color"));
	ProgramOptions::layerSegPassivLineColor.getColorDialogAction()->setText(tr("Passiv line color"));
	ProgramOptions::layerSegActiveLineColor.getColorDialogAction()->setIcon(QIcon(":/icons/color_wheel.png"));
	ProgramOptions::layerSegPassivLineColor.getColorDialogAction()->setIcon(QIcon(":/icons/color_wheel.png"));

	ProgramOptions::bscanSegmetationLineColor.getColorDialogAction()->setText(tr("change segmentation line color"));
	ProgramOptions::bscanSegmetationLineColor.getColorDialogAction()->setIcon(QIcon(":/icons/color_wheel.png"));


	ProgramOptions::layerSegFindPointMaxPoints.setDescriptions(tr("Max interpolation points"), tr("max points for spline interpolation"));
	ProgramOptions::layerSegFindPointMaxAbsError.setDescriptions(tr("max spline interpolation error"), tr("maximal difference for spline interpolation and segmentation line"));
	ProgramOptions::layerSegFindPointRemoveTol  .setDescriptions(tr("points remove tolerance"), tr("maximal alowed error for spline interpolation"));


	ProgramOptions::layerSegActiveLineSize    .setDescriptions(tr("active line size"), tr("Line size of active segmentation layer"));
	ProgramOptions::layerSegPassivLineSize    .setDescriptions(tr("passiv line size"), tr("Line size of passiv segmentation layer"));
	ProgramOptions::layerSegSplinePointSize   .setDescriptions(tr("spline point size"), tr("Point size of the spline tool"));
}
