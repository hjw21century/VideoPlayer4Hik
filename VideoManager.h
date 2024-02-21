#pragma once

#include "video/HCUsbSDK.h"
#include "video/PlayCtrl/PlayM4.h"

#define MAX_USB_DEV_LEN 64

class CVideoManager
{
public:
    static CVideoManager* GetInstance();
private:
    CVideoManager(void);
public:
    ~CVideoManager(void);

    int InitDevice();
    int LoginDevice();
    int ConfigDevice();
    BOOL StartPreview();
    void SetPlayHwnd(HWND handle);
    BOOL StartPlayFile(char* videoName, FileEndCallback fileEndCallback, void *pUser);
    void StopPlayFile();

    void StopPreview();
    void LogoutDevice();
    void UnInitDevice();

    void StartRecord(char* videoName);
    void StopRecord();

private:
    static CVideoManager* m_videoManager;
    USB_DEVICE_INFO m_aHidDevInfo[MAX_USB_DEV_LEN];
    USB_USER_LOGIN_INFO m_struCurUsbLoginInfo;

    USB_VIDEO_CAPACITY m_CamVideoCap[100];
    USB_AUDIO_PARAM    m_CamAudioCap[100];

    int  m_nHidDevNum;
    DWORD m_lUserID;
    int m_nAudioCapCount;
    int m_nVideoCapCount;
    int m_nAudioCapIndex;
    int m_nVideoCapIndex;
    int m_nVideoRateIndex;

    int m_dwSrcStreamType;
    int m_dwFrameRate;
    int m_dwWidth;
    int m_dwHeight;

    BOOL m_bPreviewAudio;
    HWND m_hVideoWnd;
    LONG m_iPreviewChannel;
    LONG m_iRecordChannel;
    LONG m_iPort;

    int GetCamVideoCap();
    int GetCamAudioCap();
    BOOL GetVideoFormat(INT32 &dwSrcStreamType, INT32 &dwFrameRate, INT32 &dwWidth, INT32 &dwHeight);
};
