/*

  FILE: DataRefRecorder.h

  BD-5J Plugin for X-Plane 11
  Copyright © 2018 by Quantumac

  GNU GENERAL PUBLIC LICENSE, Version 2, June 1991

*/

#ifndef __DATAREF_RECORDER__
#define __DATAREF_RECORDER__

//--------------------------------------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------------------------------------
#include "ValueRecorder.h"
#include "XPLMDataAccess.h"

//--------------------------------------------------------------------------------------------------------------------
// CLASS DataRefRecorder
//--------------------------------------------------------------------------------------------------------------------
template <typename T> class DataRefRecorder : public ValueRecorder<T>
{
  protected:
    string       m_dataRefName;
    XPLMDataRef  m_dataRef;
    int          m_index;
    T            m_initVal;

    //-----------------------------------------------------------------------------
    virtual T GetDataRefValue() = 0;
    virtual void SetDataRefValue(T val) = 0;

  public:

    //-----------------------------------------------------------------------------
    DataRefRecorder()
    {
      m_dataRef     = NULL;
      m_index       = -1;
      m_initVal     = 0;
    }

    //-----------------------------------------------------------------------------
    DataRefRecorder(const string &dataRefName,
                    XPLMDataRef dataRef,
                    int index = -1,
                    size_t maxReplayCount = 0,
                    T recordTolerance = 0,
                    T initVal = 0) :
      ValueRecorder<T>(maxReplayCount, recordTolerance)
    {
      m_dataRefName = dataRefName;
      m_dataRef     = dataRef;
      m_index       = index;
      m_initVal     = initVal;
    }
    
    //-----------------------------------------------------------------------------
    const char *GetDataRefName() { return m_dataRefName.c_str(); }

    //-----------------------------------------------------------------------------
    void RecordDataRef(float elapsedTime)
    {
      this->RecordValue(elapsedTime, this->GetDataRefValue());
    }

    //-----------------------------------------------------------------------------
    void ReplayDataRef(float elapsedTime)
    {
      T val;
      
      if (this->ReplayValue(elapsedTime, val))
        {
          this->SetDataRefValue(val);
        }
    }
    
    //-----------------------------------------------------------------------------
    void RestoreDataRef()
    {
      T val;
      
      if (this->GetLastRecordedValue(val))
        {
          this->SetDataRefValue(val);
        }
    }
    
    void Init()
    {
      this->Clear();
      this->RecordValue(0.0, m_initVal);
    }
};

//--------------------------------------------------------------------------------------------------------------------
// CLASS FloatDataRefRecorder
//--------------------------------------------------------------------------------------------------------------------
class FloatDataRefRecorder : public DataRefRecorder<float>
{
  protected:
  
    //-----------------------------------------------------------------------------
    virtual float GetDataRefValue()
    {
      float val;

      if (m_index >= 0)
        {
          XPLMGetDatavf(m_dataRef, &val, m_index, 1);
        }
      else
        {
          val = XPLMGetDataf(m_dataRef);
        }
      
      return val;
    }

    //-----------------------------------------------------------------------------
    virtual void SetDataRefValue(float val)
    {
      if (m_index >= 0)
        {
          XPLMSetDatavf(m_dataRef, &val, m_index, 1);
        }
      else
        {
          XPLMSetDataf(m_dataRef, val);
        }
    }

  public:
    FloatDataRefRecorder(const string &dataRefName,
                          XPLMDataRef dataRef,
                          int index = -1,
                          size_t maxReplayCount = 0.0f,
                          float recordTolerance = 0.0f,
                          float initVal = 0.0f) :
    DataRefRecorder<float>(dataRefName, dataRef, index, maxReplayCount, recordTolerance, initVal)
    {
    }
};

//--------------------------------------------------------------------------------------------------------------------
// CLASS IntDataRefRecorder
//--------------------------------------------------------------------------------------------------------------------
class IntDataRefRecorder : public DataRefRecorder<int>
{
  protected:
    //-----------------------------------------------------------------------------
    virtual int GetDataRefValue()
    {
      int val;

      if (m_index >= 0)
        {
          XPLMGetDatavi(m_dataRef, &val, m_index, 1);
        }
      else
        {
          val = XPLMGetDatai(m_dataRef);
        }
      
      return val;
    }

    //-----------------------------------------------------------------------------
    virtual void SetDataRefValue(int val)
    {
      if (m_index >= 0)
        {
          XPLMSetDatavi(m_dataRef, &val, m_index, 1);
        }
      else
        {
          XPLMSetDatai(m_dataRef, val);
        }
    }
  
  public:
    IntDataRefRecorder(const string &dataRefName,
                        XPLMDataRef dataRef,
                        int index = -1,
                        size_t maxReplayCount = 0,
                        int recordTolerance = 0,
                        int initVal = 0) :
    DataRefRecorder<int>(dataRefName, dataRef, index, maxReplayCount, recordTolerance, initVal)
    {
    }
  
};

#endif // __DATAREF_RECORDER__
