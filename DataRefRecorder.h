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
#include "DataRecorder.h"
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
// CLASS DataRefByteArrRecorder
//--------------------------------------------------------------------------------------------------------------------
class DataRefByteArrRecorder : public DataRecorder
{
  protected:
    string       m_dataRefName;
    XPLMDataRef  m_dataRef;
    vector<uint8_t>    m_initVal;

    //-----------------------------------------------------------------------------
    virtual vector<uint8_t> GetDataRefValue() = 0;
    virtual void SetDataRefValue(vector<uint8_t> val) = 0;

  public:

    //-----------------------------------------------------------------------------
    DataRefByteArrRecorder()
    {
      m_dataRef     = NULL;
      m_initVal     = vector<uint8_t>();
    }

    //-----------------------------------------------------------------------------
    DataRefByteArrRecorder(const string &dataRefName,
                    XPLMDataRef dataRef,
                    size_t maxReplayCount = 0,
                    vector<uint8_t> initVal = vector<uint8_t>()) :
      DataRecorder(maxReplayCount)
    {
      m_dataRefName = dataRefName;
      m_dataRef     = dataRef;
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
     vector<uint8_t> val;

      if (this->ReplayValue(elapsedTime, val))
        {
          this->SetDataRefValue(val);
        }
    }

    //-----------------------------------------------------------------------------
    void RestoreDataRef()
    {
      vector<uint8_t> val;

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
// CLASS ByteArrDataRefRecorder
//--------------------------------------------------------------------------------------------------------------------
class ByteArrDataRefRecorder : public DataRefByteArrRecorder
{
  protected:
    //-----------------------------------------------------------------------------
    virtual vector<uint8_t> GetDataRefValue()
    {
        size_t sz = XPLMGetDatab(m_dataRef, NULL, 0, 0);

        char src[sz];
        vector<uint8_t> val(sz);

        XPLMGetDatab(m_dataRef, &src, 0, sz);

        val.assign(src, src + sz);
      return val;
    }

    //-----------------------------------------------------------------------------
    virtual void SetDataRefValue(vector<uint8_t> val)
    {
          XPLMSetDatab(m_dataRef, &val[0], 0, val.size());
    }

  public:
    ByteArrDataRefRecorder(const string &dataRefName,
                        XPLMDataRef dataRef,
                        size_t maxReplayCount = 0,
                        vector<uint8_t> initVal = vector<uint8_t>()) :
    DataRefByteArrRecorder(dataRefName, dataRef, maxReplayCount, initVal)
    {
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
