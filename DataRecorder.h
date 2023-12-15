#ifndef __DATA_RECORDER__
#define __DATA_RECORDER__

//--------------------------------------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------------------------------------
#include <map>
#include <vector>

using namespace std;

//--------------------------------------------------------------------------------------------------------------------
// CLASS DataRecorder
//--------------------------------------------------------------------------------------------------------------------
class DataRecorder
{
  protected:
    typename std::map <float, vector<uint8_t> > m_record;
    bool                         m_lastReplayValid;
    vector<uint8_t>              m_lastReplayVal;
    size_t                       m_maxReplayCount;

  public:

    //-----------------------------------------------------------------------------
    DataRecorder(size_t maxReplayCount = 0)
    {
      //m_lastReplayVal      = {};
      m_lastReplayValid    = false;
      m_maxReplayCount     = maxReplayCount;
    }

    //-----------------------------------------------------------------------------
    bool GetLastRecordedValue( vector<uint8_t> &outVal)
    {
      typename map <float,  vector<uint8_t> >::reverse_iterator iter = m_record.rbegin();
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
    void RecordValue(float elapsedTime,  vector<uint8_t> val)
    {
      if (!m_record.empty())
        {
          typename map <float,  vector<uint8_t> >::reverse_iterator iter = m_record.rbegin();
          if (iter != m_record.rend())
            {
               vector<uint8_t> lastVal = (*iter).second;

              if (val != lastVal){
                  m_record[elapsedTime] = val;
                }
            }
        }
      else
        {
          m_record[elapsedTime] = val;
          m_lastReplayVal.clear();
          m_lastReplayValid = false;
        }

      if (m_maxReplayCount > 0)
        {
          while (m_record.size() > m_maxReplayCount)
            {
              typename map <float,  vector<uint8_t> >::iterator iter = m_record.begin();
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
    bool ReplayValue(float elapsedTime,  vector<uint8_t> &outVal)
    {
      bool changed = false;

      //
      // If elapsed time is greater than or equal to the elapsed time for the last value, use the last value
      //
      typename map <float,  vector<uint8_t> >::reverse_iterator revIter = m_record.rbegin();
      if ((revIter != m_record.rend()) && (elapsedTime >= (*revIter).first))
        {
           vector<uint8_t> val = (*revIter).second;
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
          typename map <float,  vector<uint8_t> >::iterator iter = m_record.begin();
          if ((iter != m_record.end()) && (elapsedTime <= (*iter).first))
            {
               vector<uint8_t> val = (*iter).second;
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
          typename map <float,  vector<uint8_t> >::iterator iter = m_record.lower_bound(elapsedTime);
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
                       vector<uint8_t> val = (*iter).second;

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
      m_lastReplayVal.clear();
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


#endif // __DATA_RECORDER__
