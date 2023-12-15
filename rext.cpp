/*

  FILE: rext.cpp

  Replay Extender Plugin for X-Plane 11
  Created by Todir 2021
  Derived from
  BD-5J Plugin for X-Plane 11
  Copyright © 2019 by Quantumac

  GNU GENERAL PUBLIC LICENSE, Version 2, June 1991

    Record and replay additional dataref values.

*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <queue>
#include <stdbool.h>
#include <algorithm>
#include <utility>

#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMMenus.h"

#include "DebugPrint.h"
#include "DataRefRecorder.h"

using namespace std;

static void LoadConf();
static void RegisterDrefs();

static float AfterFlightModelLoopCallBack(float   inElapsedSinceLastCall,
                                          float   inElapsedTimeSinceLastFlightLoop,
                                          int     inCounter,
                                          void    *inRefcon);


static void ClearReplayRecorders();

static void HandleRecordAndReplayOfExternalDataRefs(float totalRunningTime,
                                                    int inReplay,
                                                    int replayTransition);

static void HandleAirplaneLoaded();

static void GetConfFilePath(string &confPath);

static void GetAircraftPluginFilePath(string &pluginFilePath);

static void GetAircraftPluginTopDirPath(string &pluginDirPath);

static void RegisterPrimaryCallbacks();
static void UnregisterPrimaryCallbacks();

static void PrintRecorderStatsToLog();

static void menu_handler(void *, void *);

static const char *sPluginName                          = "Replay Extender Plugin";
static const char *sPluginSig                           =  BUILD_VERSION;//defined in makefile
static const char *sPluginDescription                   = "Replay Extender for X-Plane";

static const char *sTotalRunningTimeDataRefName         = "sim/time/total_running_time_sec";
static const char *sInReplayModeDataRefName             = "sim/time/is_in_replay";


static XPLMDataRef            sTotalRunningTimeDataRef           = NULL;
static XPLMDataRef            sInReplayModeDataRef               = NULL;

static XPLMFlightLoopID       sAfterFlightModelLoopID            = 0;
static XPLMCreateFlightLoop_t sAfterFlightModelLoop;

static int                    sWasInReplay                            = 0;
static float sIntervalBetweenAfterFlightLoopCallbacks   = 0.01;     // seconds

static vector <FloatDataRefRecorder> sXPFloatValRecorders;
static vector <IntDataRefRecorder>   sXPIntValRecorders;
static vector <ByteArrDataRefRecorder>   sXPByteArrRecorders;
static queue <pair<string, int> > inDrefs;//queue for saving datarefs until registering is possible

static const char *sMenuRef = "Replay Extender";
static const char *sStartRecordLabel = "Start Recorder";
static const char *sStopRecordLabel = "Stop Recorder";
static bool record = false;
static int g_menu_container_idx; // The index of our menu item in the Plugins menu
static int mindex = 0;
static XPLMMenuID g_menu_id; // The menu container we'll append all our menu items to


//--------------------------------------------------------------------------------------------------------------------
// PLUGIN_API XPluginStart -
//--------------------------------------------------------------------------------------------------------------------
PLUGIN_API int XPluginStart(char *outName,
                            char *outSig,
                            char *outDesc)
{

  XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);  // Use POSIX paths

  LoadConf();

  // Plugin Info
  strcpy(outName, sPluginName);
  strcpy(outSig,  sPluginSig);
  strcpy(outDesc, sPluginDescription);

    g_menu_container_idx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), sMenuRef, 0, 0);
	g_menu_id = XPLMCreateMenu(sMenuRef, XPLMFindPluginsMenu(), g_menu_container_idx, menu_handler, (void *) sMenuRef);
	//XPLMAppendMenuItem(g_menu_id, "Toggle Record", (void *)"record", 1);

	XPLMMenuID aircraft_menu = XPLMFindAircraftMenu();
	if(aircraft_menu) // This will be NULL unless this plugin was loaded with an aircraft (i.e., it was located in the current aircraft's "plugins" subdirectory)
	{
		mindex = XPLMAppendMenuItem(g_menu_id, sStartRecordLabel, (void *)"rec", 1);
        XPLMCheckMenuItem(g_menu_id, mindex, xplm_Menu_Unchecked);
	}
  //
  // Find X-Plane datarefs we need
  //

  sTotalRunningTimeDataRef            = XPLMFindDataRef(sTotalRunningTimeDataRefName);
  sInReplayModeDataRef                = XPLMFindDataRef(sInReplayModeDataRefName);


  RegisterPrimaryCallbacks();

  return 1;
}

//--------------------------------------------------------------------------------------------------------------------
// PLUGIN_API XPluginStop -
//--------------------------------------------------------------------------------------------------------------------
PLUGIN_API void XPluginStop(void)
{
  XPLMDestroyMenu(g_menu_id);
  UnregisterPrimaryCallbacks();
  PrintRecorderStatsToLog();
}

//--------------------------------------------------------------------------------------------------------------------
// XPluginDisable -
//--------------------------------------------------------------------------------------------------------------------
PLUGIN_API void XPluginDisable(void)
{
  UnregisterPrimaryCallbacks();
}

//--------------------------------------------------------------------------------------------------------------------
// XPluginEnable -
//--------------------------------------------------------------------------------------------------------------------
PLUGIN_API int XPluginEnable(void)
{
  RegisterPrimaryCallbacks();

  return 1;   // Yeah, we're here...
}

//--------------------------------------------------------------------------------------------------------------------
// PLUGIN_API RegisterPrimaryCallbacks -
//--------------------------------------------------------------------------------------------------------------------
static void RegisterPrimaryCallbacks()
{
  //
  // Register our after flight model loop callback and schedule it
  //
  if (!sAfterFlightModelLoopID)
    {
      memset(&sAfterFlightModelLoop, 0, sizeof(sAfterFlightModelLoop));
      sAfterFlightModelLoop.structSize   = sizeof(sAfterFlightModelLoop);
      sAfterFlightModelLoop.phase        = xplm_FlightLoop_Phase_AfterFlightModel;
      sAfterFlightModelLoop.callbackFunc = AfterFlightModelLoopCallBack;

      sAfterFlightModelLoopID = XPLMCreateFlightLoop(&sAfterFlightModelLoop);

      XPLMScheduleFlightLoop(sAfterFlightModelLoopID, -1, 1);
    }
}

//--------------------------------------------------------------------------------------------------------------------
// UnregisterPrimaryCallbacks -
//--------------------------------------------------------------------------------------------------------------------
void UnregisterPrimaryCallbacks()
{

  if (sAfterFlightModelLoopID)
    {
      XPLMDestroyFlightLoop(sAfterFlightModelLoopID);
      sAfterFlightModelLoopID = 0;
    }

}

//--------------------------------------------------------------------------------------------------------------------
// ClearReplayRecorders -
//--------------------------------------------------------------------------------------------------------------------
static void ClearReplayRecorders()
{
  unsigned i;

  for (i = 0; i < sXPFloatValRecorders.size(); i++)
    {
      sXPFloatValRecorders[i].Init();
    }

  for (i = 0; i < sXPIntValRecorders.size(); i++)
    {
      sXPIntValRecorders[i].Init();
    }
  for (i = 0; i < sXPByteArrRecorders.size(); i++)
    {
      sXPByteArrRecorders[i].Init();
    }

  sWasInReplay = 0;
}

//--------------------------------------------------------------------------------------------------------------------
// XPluginReceiveMessage -
//--------------------------------------------------------------------------------------------------------------------
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID    inFromWho,
                                      long            inMessage,
                                      void            *inParam)
{
  switch (inMessage)
    {
      case XPLM_MSG_PLANE_LOADED:
        {
          HandleAirplaneLoaded();
        }
        break;
      case XPLM_MSG_AIRPORT_LOADED:
      case XPLM_MSG_PLANE_UNLOADED:
      case XPLM_MSG_WILL_WRITE_PREFS:
      break;

      //default:
      // DPRINT("XPluginReceiveMessage: message %ld recieved from %u\n", inMessage, inFromWho);

    }
}

//--------------------------------------------------------------------------------------------------------------------
// HandleAirplaneLoaded -
//--------------------------------------------------------------------------------------------------------------------
static void HandleAirplaneLoaded()
{
  DPUTS("HandleAirplaneLoaded called\n");
  ClearReplayRecorders();
}

//--------------------------------------------------------------------------------------------------------------------
// AfterFlightModelLoopCallBack - our flight loop callback routine after flight model integrated
//--------------------------------------------------------------------------------------------------------------------
static float AfterFlightModelLoopCallBack(float   inElapsedSinceLastCall,
                                          float   inElapsedTimeSinceLastFlightLoop,
                                          int     inCounter,
                                          void    *inRefcon)
{
    if(!inDrefs.empty())//stop if queue is empty
    {
        RegisterDrefs();//lazy register datarefs
    }

  float totalRunningTime = XPLMGetDataf(sTotalRunningTimeDataRef);

  int inReplay         = XPLMGetDatai(sInReplayModeDataRef);
  int replayTransition = (inReplay != sWasInReplay);
  sWasInReplay         = inReplay;

    if(record == true)
    {
        HandleRecordAndReplayOfExternalDataRefs(totalRunningTime, inReplay, replayTransition);
    }



  return sIntervalBetweenAfterFlightLoopCallbacks;
}

//--------------------------------------------------------------------------------------------------------------------
// HandleRecordAndReplayOfExternalDataRefs -
//--------------------------------------------------------------------------------------------------------------------
static void HandleRecordAndReplayOfExternalDataRefs(float totalRunningTime,
                                                    int inReplay,
                                                    int replayTransition)
{
  unsigned i;

  if (replayTransition)
    {
      for (i = 0; i < sXPFloatValRecorders.size(); i++)
        {
          sXPFloatValRecorders[i].Reset();
        }

      for (i = 0; i < sXPIntValRecorders.size(); i++)
        {
          sXPIntValRecorders[i].Reset();
        }
      for (i = 0; i < sXPByteArrRecorders.size(); i++)
        {
          sXPByteArrRecorders[i].Reset();
        }
    }

  if (!inReplay)
    {
      //
      // Not in replay mode
      //

      if (replayTransition)
        {
          for (i = 0; i < sXPFloatValRecorders.size(); i++)
            {
              sXPFloatValRecorders[i].RestoreDataRef();
            }

          for (i = 0; i < sXPIntValRecorders.size(); i++)
            {
              sXPIntValRecorders[i].RestoreDataRef();
            }
          for (i = 0; i < sXPByteArrRecorders.size(); i++)
            {
              sXPByteArrRecorders[i].RestoreDataRef();
            }
        }
      else
        {
          for (i = 0; i < sXPFloatValRecorders.size(); i++)
            {
              sXPFloatValRecorders[i].RecordDataRef(totalRunningTime);
            }

          for (i = 0; i < sXPIntValRecorders.size(); i++)
            {
              sXPIntValRecorders[i].RecordDataRef(totalRunningTime);
            }
          for (i = 0; i < sXPByteArrRecorders.size(); i++)
            {
              sXPByteArrRecorders[i].RecordDataRef(totalRunningTime);
            }
        }
    }
  else
    {
      //
      // In replay mode
      //
      for (i = 0; i < sXPFloatValRecorders.size(); i++)
        {
          sXPFloatValRecorders[i].ReplayDataRef(totalRunningTime);
        }

      for (i = 0; i < sXPIntValRecorders.size(); i++)
        {
          sXPIntValRecorders[i].ReplayDataRef(totalRunningTime);
        }
      for (i = 0; i < sXPByteArrRecorders.size(); i++)
        {
          sXPByteArrRecorders[i].ReplayDataRef(totalRunningTime);
        }
    }
}


//--------------------------------------------------------------------------------------------------------------------
// GetConfFilePath - return a path to our conf file
//--------------------------------------------------------------------------------------------------------------------
static void GetConfFilePath(string &confPath)
{
  string temp;
  GetAircraftPluginTopDirPath(temp);

  confPath = temp;
  confPath += XPLMGetDirectorySeparator();
  confPath += "rextconfig.txt";
}

//--------------------------------------------------------------------------------------------------------------------
// LoadConf - load preferences
//--------------------------------------------------------------------------------------------------------------------
static void LoadConf()
{
  string confPath;
  GetConfFilePath(confPath);

    //Read conf file C++ way, skip empty lines and comments #

    fstream conffile;
    DPRINT("Loading datarefs from: %s\n",confPath.c_str());
    conffile.open(confPath,ios::in);
    if (conffile.is_open())
    {
      string line;

      while(getline(conffile, line))
      {
        if(!line.empty())
        {
          //deal with spaces and multyplatform line endings
          std::string chars = " \r\n";

          for (char c: chars) {
              line.erase(std::remove(line.begin(), line.end(), c), line.end());
          }

          if(line.empty()) continue;

          if(line.substr(0,1) == "%")//config symbol
          {
              float interval = stof(line.substr(1),nullptr);

              if(interval != NAN)
              {
                  if(interval == 0.0)
                        interval = -1.0;

                sIntervalBetweenAfterFlightLoopCallbacks = interval;
              }


            DPRINT("Recording interval set to: %f seconds\n",sIntervalBetweenAfterFlightLoopCallbacks)
          }
          else if(line.substr(0,1) == "#")//comment symbol
          {
            continue;
          }
          else
          {
            //find out if dref is in array and save index
            size_t startIndex = line.find('[');
            size_t endIndex = line.find(']');
            if (startIndex != string::npos && endIndex != string::npos)
            {
                int i = -1;
                startIndex++;
                string index;
                if(endIndex>startIndex)//check if the user made a mistake and protect stoi
                {
                    index = line.substr(startIndex, endIndex - startIndex);
                    if(!index.empty())//protect stoi
                    {
                        i = stoi(index,nullptr,10);//save index as int
                    }
                }
                inDrefs.push(make_pair(line.substr(0, startIndex-1), i));
            }
            else//if not in array dref
            {
                inDrefs.push(make_pair(line, -1));
            }
          }
        }
      }
      conffile.close();
    }
    else
    {
       DPUTS("Config file not found. Stopping...");
       XPLMDisablePlugin(XPLMGetMyID());//does it work?
    }
}

static void RegisterDrefs()
{
    static unsigned attempts = 0;
        DPRINT("Daterefs remaining %zu\n",inDrefs.size());
        for (long long unsigned i=0; i<inDrefs.size(); i++)
        {
                //If dataref exsists push it to the coresponding vecor
                XPLMDataRef temp = XPLMFindDataRef(inDrefs.front().first.c_str());
                if (temp != NULL)
                {
                    XPLMDataTypeID type = XPLMGetDataRefTypes(temp);
                    if(XPLMCanWriteDataRef(temp))//try to find out if the dataref is writable. Else ignore it.
                    {
                        //Try to guess what is the type of the dataref and register it accordingly.
                        if((type & xplmType_Float) == xplmType_Float)
                        {
                            sXPFloatValRecorders.push_back(FloatDataRefRecorder(inDrefs.front().first, temp));
                            DPRINT("Float type dateref registered %s\n",inDrefs.front().first.c_str());
                        }
                        else if((type & xplmType_Int) == xplmType_Int)
                        {
                            sXPIntValRecorders.push_back(IntDataRefRecorder(inDrefs.front().first, temp));
                            DPRINT("Int type dateref registered %s\n",inDrefs.front().first.c_str());
                        }
                        else if((type & xplmType_Data) == xplmType_Data)
                        {
                            sXPByteArrRecorders.push_back(ByteArrDataRefRecorder(inDrefs.front().first, temp));
                            DPRINT("Byte array type dateref registered %s\n",inDrefs.front().first.c_str());
                        }
                        else if((type & xplmType_FloatArray) == xplmType_FloatArray)
                        {
                            if(inDrefs.front().second >= 0)
                            {
                                string dref_name = inDrefs.front().first+"[" + to_string(inDrefs.front().second)+"]";//Restore the name with the index
                                sXPFloatValRecorders.push_back(FloatDataRefRecorder(dref_name, temp, inDrefs.front().second));
                                DPRINT("Float type array member dateref registered %s\n",dref_name.c_str());
                            }
                            else
                            {
                                //If the user missed the index of an array member dataref tell him and skip.
                                DPRINT("Dateref is type FloatArray, but no index is provided. Skipping... %s\n",inDrefs.front().first.c_str());
                            }
                        }
                        else if((type & xplmType_IntArray) == xplmType_IntArray)
                        {
                            if(inDrefs.front().second >= 0)
                            {
                                string dref_name = inDrefs.front().first+"[" + to_string(inDrefs.front().second)+"]";
                                sXPIntValRecorders.push_back(IntDataRefRecorder(dref_name, temp, inDrefs.front().second));
                                DPRINT("Int type array member dateref registered %s\n",dref_name.c_str());
                            }
                            else
                            {
                                DPRINT("Dateref is type IntArray, but no index is provided. Skipping... %s\n",inDrefs.front().first.c_str());
                            }
                        }
                        else
                        {
                            DPRINT("Dateref type not supported %s. Only float and int types are supported\n", inDrefs.front().first.c_str());

                        }
                    }
                    else
                    {
                        DPRINT("Dateref not writable and will not be used %s\n",inDrefs.front().first.c_str());
                    }

                    inDrefs.pop();//Remove the dataref we have just registered from the queue
                }
                else
                {
                    //If the dataref does not exist yet during this iteration push it at the back of the queue for the next attempt.
                    inDrefs.push(inDrefs.front());
                    inDrefs.pop();
                }

        }
        if(++attempts > 200)//Enough is enough. If a dataref does not exist/is created in, lets say 100 loops, it is probably wrong. Give up.
        {
            for(size_t i = 0; i<inDrefs.size(); i++)
            {
              DPRINT("Dataref not found in 200 flight loops. Skipping... %s\n", inDrefs.front().first.c_str());
              inDrefs.pop();
            }
        }
}


//--------------------------------------------------------------------------------------------------------------------
// GetAircraftPluginFilePath - return path to the aircraft's plugin directory
//--------------------------------------------------------------------------------------------------------------------
static void GetAircraftPluginFilePath(string &pluginFilePath)
{
  char temp[1024];
  XPLMGetPluginInfo(XPLMGetMyID(), NULL, temp, NULL, NULL);

  pluginFilePath = temp;
}

//--------------------------------------------------------------------------------------------------------------------
// GetAircraftPluginTopDirPath - return top directory of the plugin
//--------------------------------------------------------------------------------------------------------------------
static void GetAircraftPluginTopDirPath(string &pluginDirPath)
{
  string path;
  GetAircraftPluginFilePath(path);

  string dirSep = XPLMGetDirectorySeparator();

  size_t pos = path.rfind(dirSep);
  if (pos != string::npos)
    {
      pos = path.rfind(dirSep, pos - 1);
      if (pos != string::npos)
        {
            pluginDirPath = path.substr(0, pos);
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------
// PrintRecorderStatsToLog -
//--------------------------------------------------------------------------------------------------------------------
static void PrintRecorderStatsToLog()
{
  unsigned i;

  DPUTS("\n");
  DPRINT("Total Elapsed Time Recorded: %.3f\n", XPLMGetDataf(sTotalRunningTimeDataRef));

  for (i = 0; i < sXPFloatValRecorders.size(); i++)
    {
      DPRINT("%-60s has %zu recorded elements\n",
              sXPFloatValRecorders[i].GetDataRefName(), sXPFloatValRecorders[i].NumEventsRecorded());
    }

  for (i = 0; i < sXPIntValRecorders.size(); i++)
    {
      DPRINT("%-60s has %zu recorded elements\n",
              sXPIntValRecorders[i].GetDataRefName(), sXPIntValRecorders[i].NumEventsRecorded());
    }
  for (i = 0; i < sXPIntValRecorders.size(); i++)
    {
      DPRINT("%-60s has %zu recorded elements\n",
              sXPByteArrRecorders[i].GetDataRefName(), sXPByteArrRecorders[i].NumEventsRecorded());
    }
  DPUTS("\n");
}


static void menu_handler(void * in_menu_ref, void * in_item_ref)
{

	if(!strcmp((const char *)in_item_ref, "rec") && XPLMGetDatai(sInReplayModeDataRef) == 0)
	{
        XPLMMenuCheck check;
        XPLMCheckMenuItemState(g_menu_id, mindex,  &check);
		if (check == xplm_Menu_Unchecked){
            XPLMCheckMenuItem(g_menu_id, mindex, xplm_Menu_Checked);
            XPLMSetMenuItemName(g_menu_id, mindex, sStopRecordLabel, 0);
            record = true;
		}
		else if (check == xplm_Menu_Checked){
            XPLMCheckMenuItem(g_menu_id, mindex, xplm_Menu_Unchecked);
            XPLMSetMenuItemName(g_menu_id, mindex, sStartRecordLabel, 0);
            record = false;
		}
	}

}
