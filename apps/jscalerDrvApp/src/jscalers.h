#ifndef JSCALERS_DOT_H
#define JSCALERS_DOT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <pthread.h>


#include "common.h"

#ifndef SIMULATION
#include "cratemsgclient.h"
#endif

using namespace std;

///============================================================================

struct GenericSetBoard {
    int command; /// e.g. setthresholds, set_some_mode_for_channel
    vector<double> values; /// values to set
    int channelNumber; /// negative (-1) means to ussue a command for a whole board
};

///============================================================================
///============================================================================
class JlabBoard {

    private:

    protected:
        // Prevent copying boards
        JlabBoard(const JlabBoard& board);
        JlabBoard& operator=(const JlabBoard& board);
    public:

        CrateMsgClient* crateMsgClient;
        int smi_state; 
        int smi_state_prev; 

        vector<string> part_epics_names;

        int slotNumber;
        int boardType;
        int numberOfChannels;
        vector<GenericSetBoard> genericSetBoards;   
        vector< double > *scalerCounts; // order: ThreshTrig_Gr1, ThreshTDC_Gr1, ThreshTrig_Gr2, ThreshTDC_Gr2 
        // -1 (negative) means no data
        vector< double > *scalerCountsHz; // order: ScalerThreshTrig_Gr1, ScalerThreshTDC_Gr1, ScalerThreshTrig_Gr2, ScalerThreshTDC_Gr2 
        // -1 (negative) means no data or reference is zero
        vector< uint > scalerClocks; // order: SCALER_PARTYPE_THRESHOLD (Scaler Gr1), SCALER_PARTYPE_THRESHOLD2(Scaler Gr2)

        vector< double > *scalerThresholds; // order: SCALER_PARTYPE_THRESHOLD (Scaler Gr1), SCALER_PARTYPE_THRESHOLD2(Scaler Gr2)
        vector< double > *scalerModes; // not implemented

        JlabBoard(CrateMsgClient *msgClient, int slot, int type ) : crateMsgClient(msgClient), slotNumber(slot), boardType(type){
            // types: SCALER_TYPE_DSC2, SCALER_TYPE_FADC250
            smi_state_prev=SMI_STATE_UNDEFINED;
            unsigned int *buf[1];
            int len;
            int partype = SCALER_PARTYPE_NCHANNELS;
            (*buf)=0;
            int ret = msgClient->GetBoardParams(slotNumber, partype, buf, &len);
            if (ret) numberOfChannels=((*buf)[0]);
            printf("numberOfChannels===========%d slot=%d\n", numberOfChannels, slot);
            if (*buf) delete (*buf);
            if (numberOfChannels>0) {
                scalerCounts=new vector< double >[numberOfChannels];
                scalerCountsHz=new vector< double >[numberOfChannels];
                scalerThresholds=new vector< double >[numberOfChannels];
                scalerModes=new vector< double >[numberOfChannels];
            }
        }
        virtual ~JlabBoard(){};

        virtual int GetNumberOfChannels();

        virtual vector<double> *GetScalerCounts
            ( int chanNumber, bool inHz = true ); // returns scalerCountsHz or scalerCounts: order: ThreshTrig_Gr1, ThreshTDC_Gr1, ThreshTrig_Gr2, ThreshTDC_Gr2 
        // negative values means there is no data 

        virtual double GetScalerCount
            ( int chanNumber, int type, bool inHz = true ); // type range: ScalerThreshTrig_Gr1, ScalerThreshTDC_Gr1, ScalerThreshTrig_Gr2, ScalerThreshTDC_Gr2
        // returns negative if there is no data 
        virtual uint GetClockCounts( int type );  // type range:  (Scaler_Gr1), (Scaler_Gr2)

        virtual void SetThresholds( int threshType, double threshold  );  // type range: SCALER_PARTYPE_THRESHOLD (), SCALER_PARTYPE_THRESHOLD2()
        virtual void SetThreshold( int chanNumber, int threshType, double threshold  );  // type range: SCALER_PARTYPE_THRESHOLD (), SCALER_PARTYPE_THRESHOLD2()
        virtual void GetThresholds( int threshType, double threshold[]   );  // type range: SCALER_PARTYPE_THRESHOLD (), SCALER_PARTYPE_THRESHOLD2()
        virtual void GetThreshold( int chanNumber, int threshType, double & threshold  );  // type range: SCALER_PARTYPE_THRESHOLD (), SCALER_PARTYPE_THRESHOLD2()

        virtual void SetReadModes(double values[]); // not implemented completely
        virtual void SetReadMode(const unsigned chanNumber, double values[]); // not implemented completely
        virtual void GetReadModes(double values[]); // not implemented completely
        virtual void GetReadMode(const unsigned chanNumber, double values[]); // not implemented completely
};

///============================================================================
class JlabDisc2Board : public JlabBoard {
    protected:
        // Prevent copying boards
        JlabDisc2Board(const JlabDisc2Board& board);
        JlabDisc2Board& operator=(const JlabDisc2Board& board);
    public:
        JlabDisc2Board(CrateMsgClient *crateMsgClient, int slot, int type ) : JlabBoard(crateMsgClient, slot, type){}
        virtual ~JlabDisc2Board(){};

};
///============================================================================
class JlabFadc250Board : public JlabBoard {
    protected:
        // Prevent copying boards
        JlabFadc250Board(const JlabFadc250Board& board);
        JlabFadc250Board& operator=(const JlabFadc250Board& board);
    public:
        JlabFadc250Board(CrateMsgClient *crateMsgClient, int slot, int type ) : JlabBoard(crateMsgClient,slot,type){}
        virtual ~JlabFadc250Board(){};

};
///============================================================================
class JlabSSPBoard : public JlabBoard {
    protected:
        // Prevent copying boards
        JlabSSPBoard(const  JlabSSPBoard& board);
        JlabSSPBoard& operator=(const  JlabSSPBoard& board);
    public:
        static const int MAXFIBERS=8;
        static const int SCALERSPERFIBER=192;
        static const int DATAPERFIBER=5;
        static const int HEADEROFFSET=3;
        static const double CLOCKFREQ=125E6;
       
        int numOfSSPDataChannels;
        vector< double > *SSPData;
        vector< int > scalerFibers;
        vector< int > dataFibers;

        int getNumberOfScalerFibers() {
            unsigned int *buf[1];
            (*buf) = 0;
            int len,ret,ii,thisLen;
            int nChannels=0;
            ret = crateMsgClient->ReadScalers(slotNumber,buf,&len);
            if (ret) {
                if (len % (SCALERSPERFIBER+HEADEROFFSET) != 0) 
                    printf("JlabSSPBoard:  odd scaler array length1:  %d\n",len);
                ii=0;
                while (1) {
                    if (ii>=len) break;
                    thisLen = (*buf)[ii];
                    if (thisLen != HEADEROFFSET+SCALERSPERFIBER)
                        printf("JLabSSPBoard:  add scalar array length2:  %d\n",thisLen);
                    ii+=thisLen;
                    nChannels++;
                }
                if (ii!=len)
                    printf("JLabSSPBoard:  add scalar array length3:  %d/%d\n",ii,len);
            }
            return nChannels;
        }
        int getNumberOfDataFibers() {
            unsigned int *buf[1];
            (*buf) = 0;
            int len,ret,ii,thisLen;
            int nChannels=0;
            ret = crateMsgClient->ReadData(slotNumber,buf,&len);
            if (ret) {
                if (len % (DATAPERFIBER+HEADEROFFSET) != 0) 
                    printf("JlabSSPBoard:  odd scaler array length1:  %d\n",len);
                ii=0;
                while (1) {
                    if (ii>=len) break;
                    thisLen = (*buf)[ii];
                    if (thisLen != HEADEROFFSET+DATAPERFIBER)
                        printf("JLabSSPBoard:  add scalar array length2:  %d\n",thisLen);
                    ii+=thisLen;
                    nChannels++;
                }
                if (ii!=len)
                    printf("JLabSSPBoard:  add scalar array length3:  %d/%d\n",ii,len);
            }
            return nChannels;
        }

        JlabSSPBoard(CrateMsgClient *crateMsgClient, int slot, int type ) : JlabBoard(crateMsgClient,slot,type){
            unsigned int *buf[1];
            (*buf) = 0;
            
            numberOfChannels     = getNumberOfScalerFibers();
            numOfSSPDataChannels = getNumberOfScalerFibers();
            printf("JLabSSPBoard:  number of scaler channels (%d) = %d\n",slot,numberOfChannels);
            printf("JLabSSPBoard:  number of data channels (%d) = %d\n",slot,numOfSSPDataChannels);
            
            scalerCounts=new vector< double >[numberOfChannels];
            scalerCountsHz=new vector< double >[numberOfChannels];
            SSPData=new vector< double >[numOfSSPDataChannels];
/*
            int len,ret,ii,thisLen;
            numberOfChannels = 0;
            ret = crateMsgClient->ReadScalers(slotNumber,buf,&len);
            if (ret) {
                if (len % (SCALERSPERFIBER+HEADEROFFSET) != 0) 
                    printf("JlabSSPBoard:  odd scaler array length1:  %d\n",len);
                else {
                    ii=0;
                    while (1) {
                        if (ii>=len) break;
                        thisLen = (*buf)[ii];
                        if (thisLen != HEADEROFFSET+SCALERSPERFIBER)
                            printf("JLabSSPBoard:  add scalar array length2:  %d\n",thisLen);
                        ii+=thisLen;
                        numberOfChannels++;
                    }
                }
            }
            printf("JLabSSPBoard:  number of scaler channels (%d) = %d\n",slot,numberOfChannels);

            // determine number of fibers for data:
            numOfSSPDataChannels = 0;
            ret = crateMsgClient->ReadData(slotNumber,buf,&len);
            if (ret) {
                if (len % (DATAPERFIBER+HEADEROFFSET) != 0) 
                    printf("JlabSSPBoard:  odd data array length1:  %d\n",(*buf)[0]);
                else {
                    numOfSSPDataChannels = (*buf)[0] / (DATAPERFIBER+HEADEROFFSET);
                    ii=0;
                    while (1) {
                        if (ii>=len) break;
                        thisLen = (*buf)[ii];
                        if (thisLen != HEADEROFFSET+DATAPERFIBER)
                            printf("JLabSSPBoard:  odd data array length2:  %d\n",thisLen);
                        ii+=thisLen;
                        numOfSSPDataChannels++;
                    }
                }
            }
            printf("JLabSSPBoard:  number of data channels (%d) = %d\n",slot,numOfSSPDataChannels);
*/
            
            if (*buf) delete (*buf);
        }
        virtual ~JlabSSPBoard(){};

};
///============================================================================
class VmeChassis {
    private:
        string HOSTNAME;

    protected:
        // Prevent copying boards
        VmeChassis(const VmeChassis& chassis);
        VmeChassis& operator=(const VmeChassis& chassis);

    public:

        int port;  /// should be gotten from the databses (not used at the moment)
        CrateMsgClient* crateMsgClient;
        pthread_mutex_t IOmutex;// = PTHREAD_MUTEX_INITIALIZER;
        map<int, JlabBoard *> crateBoards; // key is slot
        pthread_t threadC;
        int is_crate_read;
        int numberOfSlots;
        VmeChassis(int id, string &hostname );  /// id is to talk from record
        bool IsCrateRead(){return is_crate_read;}
        int getPortFromDb(string &hostname); /// get port from db (not used now)
        int GetNumberOfSlots();
        map<int, JlabBoard*> *GetBoardMap(); // key is slot
};
///===========================================================================

class ScalersSlowControl{
    public:
        ScalersSlowControl();
        map<int, VmeChassis* > vmecrates;
        map<int, VmeChassis*> *GetChassisMap();
};

///============================================================================


#endif






