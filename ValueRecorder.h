/*

  FILE: ValueRecorder.h

  BD-5J Plugin for X-Plane 11
  Copyright © 2018 by Quantumac

  GNU GENERAL PUBLIC LICENSE, Version 2, June 1991

*/

#ifndef __VALUE_RECORDER__
#define __VALUE_RECORDER__

//--------------------------------------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------------------------------------
#include <map>
#include <math.h>

using namespace std;

//--------------------------------------------------------------------------------------------------------------------
// CLASS ValueRecorder
//--------------------------------------------------------------------------------------------------------------------
template <typename T> class ValueRecorder
{
  protected:
    typename std::map <float, T> m_record;
    bool                         m_lastReplayValid;
    T                            m_lastReplayVal;
    T                            m_recordTolerance;
    size_t                       m_maxReplayCount;

  public:

    //-----------------------------------------------------------------------------
    ValueRecorder(size_t maxReplayCount = 0, T recordTolerance = 0)
    {
      m_lastReplayVal      = 0;
      m_lastReplayValid    = false;
      m_recordTolerance    = recordTolerance;
      m_maxReplayCount     = maxReplayCount;
    }
    
    //-----------------------------------------------------------------------------
    bool GetLastRecordedValue(T &outVal)
    {
      typename map <float, T>::reverse_iterator iter = m_record.rbegin();
      if (iter != m_record.rend())
        {
          outVal = (*iter).second;
          return true;
        }
      else
        {
          return false;
        }
    }

    //-----------------------------------------------------------------------------
    void RecordValue(float elapsedTime, T val)
    {
      if (!m_record.empty())
        {
          typename map <float, T>::reverse_iterator iter = m_record.rbegin();
          if (iter != m_record.rend())
            {
              T lastVal = (*iter).second;
              
              T diff = val - lastVal;
              if (diff < 0)
                {
                  diff = -diff;
                }
                
              if (diff > m_recordTolerance)
                {
                  m_record[elapsedTime] = val;
                }
            }
        }
      else
        {
          m_record[elapsedTime] = val;
          m_lastReplayVal = 0;
          m_lastReplayValid = false;
        }

      if (m_maxReplayCount > 0)
        {
          while (m_record.size() > m_maxReplayCount)
            {
              typename map <float, T>::iterator iter = m_record.begin();
              if (iter != m_record.end())
                {
                  m_record.erase(iter);
                }
              else
                break;
            }
        }
    }
    
    //-----------------------------------------------------------------------------
    bool ReplayValue(float elapsedTime, T &outVal)
    {
      bool changed = false;

      //
      // If elapsed time is greater than or equal to the elapsed time for the last value, use the last value
      //
      typename map <float, T>::reverse_iterator revIter = m_record.rbegin();
      if ((revIter != m_record.rend()) && (elapsedTime >= (*revIter).first))
        {
          T val = (*revIter).second;
          if ((!m_lastReplayValid) || (val != m_lastReplayVal))
            {
              changed = true;
              outVal = val;
              m_lastReplayVal = outVal;
              m_lastReplayValid = true;
            }
        }

      if (!changed)
        {
          //
          // If the elapsed time is less than or equal to elapsed time for the first value, use the first value
          //
          typename map <float, T>::iterator iter = m_record.begin();
          if ((iter != m_record.end()) && (elapsedTime <= (*iter).first))
            {
              T val = (*iter).second;
              if ((!m_lastReplayValid) || (val != m_lastReplayVal))
                {
                  changed = true;
                  outVal = val;
                  m_lastReplayVal = val;
                  m_lastReplayValid = true;
                }
            }
        }

      if (!changed)
        {
          //
          // Find the corresponding elapsed time somewhere in the middle
          //
          typename map <float, T>::iterator iter = m_record.lower_bound(elapsedTime);
          if (iter != m_record.end())
            {
              if (iter != m_record.begin())
                {
                  if ((*iter).first > elapsedTime)
                    {
                      iter--;  // Find the one at the same time or before
                    }

                  if (elapsedTime >= (*iter).first)
                    {
                      T val = (*iter).second;
                      
                      if ((!m_lastReplayValid) || (val != m_lastReplayVal))
                        {
                          changed = true;
                          outVal = val;
                          m_lastReplayVal = val;
                          m_lastReplayValid = true;
                        }
                    }
                }
            }
        }

      return changed;
    }

    //-----------------------------------------------------------------------------
    void Reset()
    {
      m_lastReplayVal = 0;
      m_lastReplayValid = false;
    }

    //-----------------------------------------------------------------------------
    void Clear()
    {
      m_record.clear();
      this->Reset();
    }
    
    //-----------------------------------------------------------------------------
    size_t NumEventsRecorded()
    {
      return m_record.size();
    }
};

#endif // __VALUE_RECORDER__

