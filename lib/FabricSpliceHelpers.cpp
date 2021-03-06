//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceEditorWidget.h" // [pzion 20150519] Must come first because of some stupid macro definition somewhere
#include "FabricSpliceHelpers.h"
#include <DFG/DFGLogWidget.h>
#include <SceneHub/DFG/SHDFGCombinedWidget.h>
#include <Licensing/Licensing.h>
#include <FabricSplice.h>
#include <FabricUI/SplashScreens/FabricSplashScreen.h>

#include <maya/MGlobal.h>
#include <maya/MFileIO.h>

#include <FTL/StrRef.h>
#include <FTL/JSONDec.h>
#include <FTL/JSONValue.h>

MString gLastLoadedScene;
MString mayaGetLastLoadedScene()
{
  return gLastLoadedScene;
}

MString gModuleFolder;
void initModuleFolder(MString moduleFolder)
{
  gModuleFolder = moduleFolder;
}

MString getModuleFolder()
{
  return gModuleFolder;
}

void mayaLogFunc(const MString & message)
{
  MGlobal::displayInfo(MString("[Fabric] ")+message);
  FabricUI::DFG::DFGLogWidget::log(message.asChar());
}

void mayaLogFunc(const char * message, unsigned int length)
{
  mayaLogFunc(MString(message));
}

bool gErrorEnabled = true;
void mayaErrorLogEnable(bool enable)
{
  gErrorEnabled = enable;
}

bool gErrorOccured = false;
void mayaLogErrorFunc(const MString & message)
{
  if(!gErrorEnabled)
    return;
  MGlobal::displayError(MString("[Fabric] ")+message);
  FabricUI::DFG::DFGLogWidget::log(message.asChar());
  gErrorOccured = true;
}

void mayaLogErrorFunc(const char * message, unsigned int length)
{
  mayaLogErrorFunc(MString(message));
}

void mayaClearError()
{
  gErrorOccured = false;
}

MStatus mayaErrorOccured()
{
  MStatus result = MS::kSuccess;
  if(gErrorOccured)
    result = MS::kFailure;
  gErrorOccured = false;
  return result;
}

void mayaKLReportFunc(const char * message, unsigned int length)
{
  if (length <= 1000)
    MGlobal::displayInfo(MString("[KL]: ")+MString(message));
  else
  {
    MString cropped(message, 1000);
    MGlobal::displayInfo(MString("[KL]: ")+cropped+MString(" [long message, only first 1000 characters displayed]"));
  }
}

void mayaCompilerErrorFunc(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc)
{
  MString line;
  line.set(row);
  MString composed = "[KL Compiler "+MString(level)+"]: line "+line+", op '"+MString(file)+"': "+MString(desc);
  MGlobal::displayInfo(composed);
  FabricSpliceEditorWidget::reportAllCompilerError(row, col, file, level, desc);
  FabricUI::DFG::DFGLogWidget::log(composed.asChar());
}

void mayaKLStatusFunc(const char * topicData, unsigned int topicLength,  const char * messageData, unsigned int messageLength)
{
  /* [FE-6245] we don't log the KL status messages.
     MString composed = MString("[KL Status]: ")+MString(messageData, messageLength);
     MGlobal::displayInfo(composed); */

  FTL::StrRef topic( topicData, topicLength );
  FTL::StrRef message( messageData, messageLength );
  FabricCore::Client *client =
    const_cast<FabricCore::Client *>( FabricSplice::DGGraph::getClient() );
  if ( topic == FTL_STR( "licensing" ) )
  {
    try
    {
      if (MGlobal::mayaState() == MGlobal::kInteractive)
      {
        FabricUI_HandleLicenseData(
          NULL,
          *client,
          message,
          false // modalDialogs
          );
      }
    }
    catch ( FabricCore::Exception e )
    {
      FabricUI::DFG::DFGLogWidget::log(e.getDesc_cstr());
    }
  }
  else if( topic == FTL_STR( "slowOp.push"))
  {
    mayaSlowOpFunc(messageData, messageLength);
  }

  // else
  //   FabricUI::DFG::DFGLogWidget::log(composed.asChar());
}

void mayaSlowOpFunc(const char *descCStr, unsigned int descLength)
{
  FabricSplashScreen * splash = FabricSplashScreen::getSplashScreen(false /*create*/);
  if(splash)
  {
    splash->setMessage(descCStr);
  }
}

void mayaRefreshFunc()
{
  MGlobal::executeCommandOnIdle("refresh");
}

void mayaSetLastLoadedScene(MString scene)
{
  gLastLoadedScene = scene;
}

bool mayaShowSplashScreen()
{
#if MAYA_API_VERSION >= 201600
  bool result = MGlobal::mayaState() == MGlobal::kInteractive;
  if(result)
  {
    result = !MFileIO::isOpeningFile();
  }
  return result;
#else
  return false;
#endif
}
