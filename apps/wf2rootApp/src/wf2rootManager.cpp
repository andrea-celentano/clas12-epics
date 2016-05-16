/*
 * wf2rootManager.cpp
 *
 *  Created on: Mar 24, 2014
 *      Author: yqiang
 * 
 *  Modified by: Wesley Moore (wmoore@jlab.org)
 *  Date: Mar 2016
 */

#include "wf2rootManager.hh"
#include "wf2rootFile.hh"

wf2rootManager* wf2rootManager::selfPtr = 0;
Int_t wf2rootManager::prmCompressFactor = 0; // no compression
std::queue<recordBuffer> wf2rootManager::prmBuffer;

UInt_t wf2rootFile::prfTreeBuffSize = static_cast<UInt_t>(1 << 16);
//UInt_t wf2rootFile::prfAutoSaveSize = static_cast<UInt_t>(1 << 30);
// the documentation is not clear: positive number means number of events
// negative number means number of bytes
UInt_t wf2rootFile::prfAutoSaveSize = static_cast<UInt_t>(1800);
string wf2rootFile::prfBranchName = "record";

// constructor
wf2rootManager::wf2rootManager() :
		prmPrefix(""), prmSuffix(""), prmThread(0), prmStop(true), prmFile(0), prmDir(
				""), prmFileName("nofile"), prmFileLimit(1e9), prmFileSize(0) {

	// reinitialize self pointer
	if (selfPtr != 0)
		delete selfPtr;
	selfPtr = this;

	// clear FIFO
	while (prmBuffer.size() > 0)
		prmBuffer.pop();

	// Enable RootSpy
	//	DRootSpy *spy = new DRootSpy();

	// Create mutex for buffer operation
	if (prmMutex)
		delete prmMutex;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	prmMutex = new pthread_mutex_t;
	pthread_mutex_init(prmMutex, &attr);

	return;
}

// destructor
wf2rootManager::~wf2rootManager() {
	if (prmFile != 0 && prmFile->IsOpen()) {
		prmFile->Close();
		prmFile = 0;
	}
	selfPtr = 0;
	return;
}

void wf2rootManager::StartDAQ(string dir, string prefix, string suffix,
		long nlim) {
	prmDir = dir;
	prmPrefix = prefix;
	prmSuffix = suffix;
	prmFileLimit = nlim;
	prmStop = false;
	pthread_create(&prmThread, NULL, WriteThread, (void*) this);

	return;
}

void wf2rootManager::NewRootFile() {
	// new filename
	TDatime dtTime;
	TString rsTime(dtTime.AsSQLString());
	rsTime.ReplaceAll(" ", "_");
	rsTime.ReplaceAll(":", "");
	rsTime.ReplaceAll("-", "");
	prmFileName = prmPrefix + string(rsTime.Data()) + prmSuffix + ".root";
	string fullpath = prmDir + prmFileName;
	prmFile = new wf2rootFile(fullpath.c_str(), "RECREATE", "WF2 ROOT File",
			prmCompressFactor);
}

void wf2rootManager::WriteFile(const char* channelName,
		struct timespec* timeStamp, float* buffPtr, int nElm) {

	recordBuffer tmprecord(string(channelName),
			wf2rootRecord(*timeStamp, nElm, buffPtr));

	// push to buffer
	pthread_mutex_lock(prmMutex);
	prmBuffer.push(tmprecord);
	pthread_mutex_unlock(prmMutex);

	return;
}

void wf2rootManager::CloseFile() {
	prmStop = true;
	if (prmThread) {
		pthread_join(prmThread, NULL);
		prmThread = 0;
	}
}

void *WriteThread(void *argument) {

	// sleep timer
	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = (long int) (0.001 * 1.e9);

	wf2rootManager *prmPtr = (wf2rootManager*) argument;

	// CLose Existing Root file
	if (prmPtr->prmFile != 0 && prmPtr->prmFile->IsOpen())
		prmPtr->prmFile->Close();

	while (!prmPtr->prmStop) {

		// read buffer if non-empty
		if (prmPtr->prmBuffer.size()) {
			pthread_mutex_lock(prmPtr->prmMutex);
			recordBuffer tmprecord = prmPtr->prmBuffer.front();
			prmPtr->prmBuffer.pop();
			pthread_mutex_unlock(prmPtr->prmMutex);

			// check if file open
			if (prmPtr->prmFile == 0 || !prmPtr->prmFile->IsOpen()) {
			  //	pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
				prmPtr->NewRootFile();
				//	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
			}

			// write to disk
			// pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
			prmPtr->prmFile->FillTree(tmprecord.treename, tmprecord.record);
			// pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

			// check file size, close if larger than tolerance
			prmPtr->prmFileSize = prmPtr->prmFile->GetEND();
			if (prmPtr->prmFileSize >= prmPtr->prmFileLimit) {
			  // pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
				prmPtr->prmFile->Close();
				// pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
			}
		} else
			nanosleep(&sleeptime, &sleeptime);
	}

	// clear buffer
	while (prmPtr->prmBuffer.size()) {
		pthread_mutex_lock(prmPtr->prmMutex);
		recordBuffer tmprecord = prmPtr->prmBuffer.front();
		prmPtr->prmBuffer.pop();
		pthread_mutex_unlock(prmPtr->prmMutex);
		// check if file open
		if (prmPtr->prmFile == 0 || !prmPtr->prmFile->IsOpen()) {
		  // pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
			prmPtr->NewRootFile();
			//	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		}
		// write to disk
		// pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
		prmPtr->prmFile->FillTree(tmprecord.treename, tmprecord.record);
		// pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
	}

	// pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
	if (prmPtr->prmFile != 0 && prmPtr->prmFile->IsOpen())
		prmPtr->prmFile->Close();
	prmPtr->prmFile = 0;
	// pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
	prmPtr->prmFileName = string("nofile");
	prmPtr->prmFileSize = 0;
	return NULL;
}

extern "C" {

#include "wf2rootManager.h"

void StartRootDAQ(const char* dir, const char* prefix, const char* suffix,
		long nlim) {
	string strdir(dir);
	string strpfx(prefix);
	string strsfx(suffix);
	wf2rootManager::GetInstance()->StartDAQ(strdir, strpfx, strsfx, nlim);
	return;
}

void CloseRootFile() {
	wf2rootManager::GetInstance()->CloseFile();
	return;
}

void WriteRootFile(const char* channelName, struct timespec* timeStamp,
		float* buffPtr, int nElm) {
	wf2rootManager::GetInstance()->WriteFile(channelName, timeStamp, buffPtr,
			nElm);
	return;
}

int GetBufferSize() {
	return wf2rootManager::GetInstance()->prmBuffer.size();
}

const char* GetFileName() {
	return wf2rootManager::GetInstance()->prmFileName.c_str();
}

long GetFileSize() {
	return wf2rootManager::GetInstance()->prmFileSize;
}
}
;
