#include "pch.h"
#include "VideoManager.h"

CVideoManager* CVideoManager::m_videoManager = NULL;

CVideoManager* CVideoManager::GetInstance()
{
    if (!m_videoManager) {
        m_videoManager = new CVideoManager();
    }
    return m_videoManager;
}

CVideoManager::CVideoManager(void)
{
    m_lUserID = USB_INVALID_USERID;

    m_nAudioCapIndex = 0;
    m_nVideoCapIndex = 0;
    m_nVideoRateIndex = 0;
    m_bPreviewAudio = FALSE;

    m_iPort = -1;
    m_iPreviewChannel = -1;
    m_iRecordChannel = -1;

    memset(m_CamVideoCap, 0, sizeof(m_CamVideoCap));
    memset(m_CamAudioCap, 0, sizeof(m_CamAudioCap));
}

CVideoManager::~CVideoManager(void)
{
}

int CVideoManager::InitDevice(void)
{
    BOOL ret = USB_Init();
	char* szLogPath = "./USBSDKLog/";
	USB_SetLogToFile(ENUM_INFO_LEVEL, szLogPath, TRUE);

    DWORD dwVersion = USB_GetSDKVersion();
    CString csSDKVersion;
    csSDKVersion.Format("HCUsbSDK V%d.%d.%d.%d", (0xff000000 & dwVersion)>>24, (0x00ff0000 & dwVersion)>>16,\
        (0x0000ff00 & dwVersion)>>8, (0x000000ff & dwVersion));//%.d,0x0000ff & dwVersion,build NO. do not expose

    DWORD dwError;
    m_nHidDevNum = USB_GetDeviceCount();
    if (m_nHidDevNum > 0)
    {
        if (USB_EnumDevices(m_nHidDevNum, &m_aHidDevInfo[0]))
        {
        }
        else
        {
            dwError = USB_GetLastError();
            return -1;
        }
    }
    else
    {
        dwError = USB_GetLastError();
        return -2;
    }

    return 0;
}

int CVideoManager::LoginDevice()
{
    char szUserName[MAX_USERNAME_LEN] = {"admin"};
    char szPassword[MAX_PASSWORD_LEN] = {"12345"};
    int ret = 0;

    USB_DEVICE_INFO struUsbDeviceInfo = {0};
    struUsbDeviceInfo.dwSize = sizeof(USB_DEVICE_INFO);
    memcpy(&struUsbDeviceInfo, &m_aHidDevInfo[0], sizeof(USB_DEVICE_INFO));

    memset(&m_struCurUsbLoginInfo, 0, sizeof(m_struCurUsbLoginInfo));
    m_struCurUsbLoginInfo.dwSize = sizeof(USB_USER_LOGIN_INFO);
    m_struCurUsbLoginInfo.dwTimeout = 5000;//登录超时时间(5秒)
    m_struCurUsbLoginInfo.dwVID = struUsbDeviceInfo.dwVID;
    m_struCurUsbLoginInfo.dwPID = struUsbDeviceInfo.dwPID;

    memcpy(m_struCurUsbLoginInfo.szSerialNumber, struUsbDeviceInfo.szSerialNumber, MAX_SERIAL_NUMBER_LEN);
    memcpy(m_struCurUsbLoginInfo.szUserName, szUserName, strlen(szUserName));
    memcpy(m_struCurUsbLoginInfo.szPassword, szPassword, strlen(szPassword));

    // 默认账号登陆
    m_struCurUsbLoginInfo.byLoginMode = 0;
    m_struCurUsbLoginInfo.dwDevIndex = 1;

    USB_DEVICE_REG_RES struDeviceRegRes = {0};
    struDeviceRegRes.dwSize = sizeof(struDeviceRegRes);

    LONG lUserIDTemp = m_lUserID;

    m_lUserID = USB_Login(&m_struCurUsbLoginInfo, &struDeviceRegRes);

    if (m_lUserID < 0) {
        return -10;
    }

    ret = GetCamVideoCap();
    if (ret < 0) {
        return ret;
    }
    
    ret = GetCamAudioCap();
    if (ret < 0) {
        return ret;
    }

    return 0;
}

void CVideoManager::LogoutDevice()
{
    if (m_lUserID >= 0) {
        USB_Logout(m_lUserID);
        m_lUserID = USB_INVALID_USERID;
    }
}

void CVideoManager::UnInitDevice()
{
    USB_Cleanup();
}


int CVideoManager::GetCamVideoCap()
{
    USB_CONFIG_INPUT_INFO struConfigInputInfo = { 0 };
    USB_CONFIG_OUTPUT_INFO struConfigOutputInfo = { 0 };

    if (m_lUserID >= 0)
    {
        memset(&m_CamVideoCap, 0, sizeof(m_CamVideoCap));
        struConfigOutputInfo.lpOutBuffer = m_CamVideoCap;
        struConfigOutputInfo.dwOutBufferSize = sizeof(m_CamVideoCap);
        m_nVideoCapCount = 0;
        //获取设备支持的视频格式
        if (!USB_GetDeviceConfig(m_lUserID, USB_GET_VIDEO_CAP, &struConfigInputInfo, &struConfigOutputInfo))
        {
            return -1;
        }
        //设备支持的视频格式的个数
        for (int i = 0; m_CamVideoCap[i].lListSize != 0; i++)
        {
            m_nVideoCapCount++;
        }
    }
    return 0;
}

int CVideoManager::GetCamAudioCap()
{
    USB_CONFIG_INPUT_INFO struConfigInputInfo = { 0 };
    USB_CONFIG_OUTPUT_INFO struConfigOutputInfo = { 0 };

    if (m_lUserID >= 0)
    {
        memset(&m_CamAudioCap, 0, sizeof(m_CamAudioCap));
        m_nAudioCapCount = 0;
        struConfigOutputInfo.lpOutBuffer = m_CamAudioCap;
        struConfigOutputInfo.dwOutBufferSize = sizeof(m_CamAudioCap);
        //获取设备支持的音频格式
        if (!USB_GetDeviceConfig(m_lUserID, USB_GET_AUDIO_CAP, &struConfigInputInfo, &struConfigOutputInfo))
        {
            return -1;
        }
        //设备支持的音频格式的个数
        for (int i = 0; (m_CamAudioCap[i].nAvgBytesPerSec && m_CamAudioCap[i].nChannels); i++)
        {
            m_nAudioCapCount++;
        }
    }
    return 0;
}

BOOL CVideoManager::GetVideoFormat(INT32 &dwSrcStreamType, INT32 &dwFrameRate, INT32 &dwWidth, INT32 &dwHeight)
{
    if (m_nVideoCapCount == 0)
    {
        return FALSE;
    }

    if (m_nVideoCapCount < 7 + 1) {
        m_nVideoCapIndex = 0;
    } else {
        // 1080P
        m_nVideoCapIndex = 7;
    }

    dwSrcStreamType = m_CamVideoCap[m_nVideoCapIndex].nType;
    dwFrameRate = m_CamVideoCap[m_nVideoCapIndex].lFrameRates[m_nVideoRateIndex];
    dwWidth = m_CamVideoCap[m_nVideoCapIndex].dwWidth;
    dwHeight = m_CamVideoCap[m_nVideoCapIndex].dwHeight;

    return TRUE;
}

int CVideoManager::ConfigDevice()
{
    // TODO:  在此添加控件通知处理程序代码

    USB_CONFIG_INPUT_INFO struConfigInputInfo = { 0 };
    USB_CONFIG_OUTPUT_INFO struConfigOutputInfo = { 0 };
    INT32 nPreviewType = USB_STREAM_PS_H264;

    if (m_bPreviewAudio)
    {
        memset(&struConfigInputInfo, 0, sizeof(struConfigInputInfo));
        memset(&struConfigOutputInfo, 0, sizeof(struConfigOutputInfo));

        struConfigInputInfo.lpInBuffer = &m_CamAudioCap[m_nAudioCapIndex];
        struConfigInputInfo.dwInBufferSize = sizeof(USB_AUDIO_PARAM);

        if (!USB_SetDeviceConfig(m_lUserID, USB_SET_AUDIO_PARAM, &struConfigInputInfo, &struConfigOutputInfo))
        {
            return -1;
        }
    }

    GetVideoFormat(m_dwSrcStreamType, m_dwFrameRate, m_dwWidth, m_dwHeight);

    USB_VIDEO_PARAM struVideoParam = { 0 };
    struVideoParam.dwVideoFormat = m_dwSrcStreamType;
    struVideoParam.dwFramerate = m_dwFrameRate;
    struVideoParam.dwWidth = m_dwWidth;
    struVideoParam.dwHeight = m_dwHeight;

    memset(&struConfigInputInfo, 0, sizeof(struConfigInputInfo));
    memset(&struConfigOutputInfo, 0, sizeof(struConfigOutputInfo));

    struConfigInputInfo.lpInBuffer = &struVideoParam;
    struConfigInputInfo.dwInBufferSize = sizeof(struVideoParam);

    //设置设备视频参数
    if (!USB_SetDeviceConfig(m_lUserID, USB_SET_VIDEO_PARAM, &struConfigInputInfo, &struConfigOutputInfo))
    {
        return -2;
    }

     //更新需要设置的参数
     USB_SRC_STREAM_CFG struSrcStreamCfg = { 0 };
     struSrcStreamCfg.dwStreamType = m_dwSrcStreamType;
     struSrcStreamCfg.bUseAudio = m_bPreviewAudio;
 
     memset(&struConfigInputInfo, 0, sizeof(struConfigInputInfo));
     memset(&struConfigOutputInfo, 0, sizeof(struConfigOutputInfo));
 
     struConfigInputInfo.lpInBuffer = &struSrcStreamCfg;
     struConfigInputInfo.dwInBufferSize = sizeof(struSrcStreamCfg);
 
     //设置设备视频参数
     if (!USB_SetDeviceConfig(m_lUserID, USB_SET_SRC_STREAM_TYPE, &struConfigInputInfo, &struConfigOutputInfo))
     {
         return -3;
     }
     return 0;
}

BOOL CVideoManager::StartPreview()
{
    // TODO:  在此添加控件通知处理程序代码
    //启动预览
    DWORD dwError = 0;
    INT32 nPreviewTypeIndex = 0;
    INT32 nPreviewType = USB_STREAM_PS_H264;
    switch (nPreviewTypeIndex)
    {
    case 0:
        nPreviewType = USB_STREAM_PS_H264;
        break;
    case 1:
        nPreviewType = USB_STREAM_PS_MJPEG;
        break;
    case 2:
        nPreviewType = USB_STREAM_PS_YUY2;
        break;
    case 3:
        nPreviewType = USB_STREAM_PS_NV12;
        break;
    default:
        break;
    }
    
    USB_PREVIEW_PARAM struPreviewParam = { 0 };
    struPreviewParam.dwStreamType = nPreviewType;
    struPreviewParam.bUseAudio = m_bPreviewAudio;
    struPreviewParam.hWindow = m_hVideoWnd;
    struPreviewParam.dwSize = sizeof(USB_PREVIEW_PARAM);
    m_iPreviewChannel = USB_StartPreview(m_lUserID, &struPreviewParam);
    if (m_iPreviewChannel == -1)
    {
        dwError = USB_GetLastError();
        return FALSE;
    } else {
        return TRUE;
    }
    // StartRecord();
}

void CVideoManager::StopPreview()
{
    if (m_iPreviewChannel >= 0)
    {
        USB_StopChannel(m_lUserID, m_iPreviewChannel);
        m_iPreviewChannel = -1;
    }
    // StopRecord();
}

void CVideoManager::StartRecord(char* videoName)
{
    DWORD dwError = 0;
    
    char path[MAX_PATH] = {0};
    GetCurrentDirectory(MAX_PATH, path);
    strcat_s(path, MAX_PATH, "\\");
    strcat_s(path, MAX_PATH, videoName);
    CString szpath(path);
    szpath.Replace("FormatConversion", "Record");
    USB_RECORD_PARAM struRecordParam = { 0 };
    strcpy_s((char*)struRecordParam.szFilePath, MAX_FILE_PATH_LEN, szpath.GetBuffer());
    struRecordParam.dwRecordType = USB_RECORDTYPE_MP4;
    struRecordParam.dwSize = sizeof(struRecordParam);
    struRecordParam.bRecordAudio = FALSE;
    m_iRecordChannel = USB_StartRecord(m_lUserID, &struRecordParam);
    if (m_iRecordChannel < 0)
    {
        dwError = USB_GetLastError();
    }
}

void CVideoManager::StopRecord()
{
    if (m_iRecordChannel >= 0)
    {
        USB_StopChannel(m_lUserID, m_iRecordChannel);
        m_iRecordChannel = -1;
    }
}

void CVideoManager::SetPlayHwnd(HWND handle)
{
    m_hVideoWnd = handle;
}

BOOL CVideoManager::StartPlayFile(char* videoName, FileEndCallback fileEndCallback, void *pUser)
{
    DWORD dwError;
    BOOL ret;

    ret = PlayM4_GetPort(&m_iPort);
    if (!ret) {
        return FALSE;
    }

    ret = PlayM4_SetFileEndCallback(m_iPort, fileEndCallback, pUser);
    if (!ret) {
        PlayM4_FreePort(m_iPort);
        return FALSE;
    }
    
    ret = PlayM4_SetStreamOpenMode(m_iPort, STREAME_REALTIME);
    if (!ret) {
        PlayM4_FreePort(m_iPort);
        return FALSE;
    }

    // 使用绝对路径
    //char path[MAX_PATH] = {0};
    //GetCurrentDirectory(MAX_PATH, path);
    //strcat_s(path, MAX_PATH, "\\");
    //strcat_s(path, MAX_PATH, videoName);
    //CString szpath(path);
    //szpath.Replace("FormatConversion", "");
    ret = PlayM4_OpenFile(m_iPort, videoName);
    if (!ret) {
        dwError = PlayM4_GetLastError(m_iPort);
        PlayM4_FreePort(m_iPort);
        return FALSE;
    }

    ret = PlayM4_Play(m_iPort, m_hVideoWnd);
    if (!ret) {
        PlayM4_CloseFile(m_iPort);
        PlayM4_FreePort(m_iPort);
        return FALSE;
    }

    return TRUE;
}

void CVideoManager::StopPlayFile()
{
    if (m_iPort < 0) {
        return;
    }
    BOOL ret;

    ret = PlayM4_Stop(m_iPort);
    if (!ret) {
        return;
    }

    ret = PlayM4_CloseFile(m_iPort);
    if (!ret) {
        return;
    }

    ret = PlayM4_FreePort(m_iPort);
    if (!ret) {
        m_iPort = -1;
        return;
    }

    /* ret = PlayM4_SetPlayedTimeEx(m_iPort, PlayM4_GetFileTime(m_iPort));
    if (!ret) {
        return;
    } */
}
