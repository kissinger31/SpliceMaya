//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <maya/MGlobal.h>
#include "FabricSpliceHelpers.h"
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>
#include <FabricUI/Commands/BaseRTValScriptableCommand.h>

using namespace FabricUI;
using namespace Commands;

bool CommandManagerMayaCallback::s_instanceFlag = false;
CommandManagerMayaCallback* CommandManagerMayaCallback::s_cmdManagerMayaCallback = 0;

CommandManagerMayaCallback::CommandManagerMayaCallback()
  : QObject()
{
  try
  {
    CommandManager *manager =  CommandManager::getCommandManager();

    QObject::connect(
      manager,
      SIGNAL(commandPushed(Command *)),
      this,
      SLOT(onCommandPushed(Command *))
      );
  }
  catch (std::string &e) 
  {
    mayaLogErrorFunc(
      QString(
        QString("CommandManagerMayaCallback::CommandManagerMayaCallback, exception: ") + 
        e.c_str()
        ).toUtf8().constData()
      );
  }
}

CommandManagerMayaCallback::~CommandManagerMayaCallback()
{
  s_instanceFlag = false;
}

CommandManagerMayaCallback *CommandManagerMayaCallback::GetCommandManagerMayaCallback()
{
  if(!s_instanceFlag)
  {
    s_cmdManagerMayaCallback = new CommandManagerMayaCallback();
    s_instanceFlag = true;
  }
  return s_cmdManagerMayaCallback;
}

inline void encodeArg(
  const QString &arg,
  std::stringstream &cmdArgs)
{
  cmdArgs << ' ';
  cmdArgs << arg.toUtf8().constData();
}
 
inline void encodeRTValArg(
  const QString &arg,
  std::stringstream &cmdArgs)
{
  cmdArgs << ' ';
  cmdArgs << "\"" << arg.toUtf8().constData() << "\"";
}

void CommandManagerMayaCallback::onCommandPushed(
  BaseCommand *cmd)
{
  // Construct a Maya 'FabricCommand'  
  // that represents the Fabric command.
  std::stringstream fabricCmd;

  // Maya command name.
  fabricCmd << "FabricCommand";

  // Fabric command name.
  encodeArg(cmd->getName(), fabricCmd);
   
  // Fabric command args.
  BaseScriptableCommand *scriptCmd = qobject_cast<BaseScriptableCommand*>(cmd);

  if(scriptCmd)
  {
    // Check if it's a BaseRTValScriptableCommand,
    // to know how to cast the string.
    BaseRTValScriptableCommand *rtValScriptCmd = qobject_cast<BaseRTValScriptableCommand*>(cmd);
     foreach(QString key, scriptCmd->getArgKeys())
    {
      encodeArg(key, fabricCmd);
      if(rtValScriptCmd)
        encodeRTValArg(scriptCmd->getArg(key), fabricCmd);
      else
        encodeArg(scriptCmd->getArg(key), fabricCmd);
    }
  }

  // Create the maya command.
  MGlobal::executeCommandOnIdle(fabricCmd.str().c_str(), true);
}
