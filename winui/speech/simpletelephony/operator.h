// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright ? Microsoft Corporation. All rights reserved

/****************************************************************************
*	Operator.h
*		Definition of the COperator class
*****************************************************************************/

#ifndef MAXULONG_PTR
#error This sample application requires a newer version of the Platform SDK than you have installed.
#endif // MAXULONG_PTR

// Disable the false warnings caused by the wrong definitions of SetWindowLongPtr
// and GetWindowLongPtr macros.
#pragma warning ( disable : 4244 )

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "callnot.h"

// Global data
const WCHAR gszTelephonySample[] = L"SAPI 5.0 Telephony sample";

class COperator
{
public:
    COperator::COperator( HINSTANCE hInst ) : 
                                m_hDlg( NULL ),
                                m_pTapi( NULL ),
                                m_pCall( NULL ),
                                m_fAutoAnswer( FALSE ),
                                m_pTAPIEventNotification( NULL )
    {};
    COperator::~COperator();

    HRESULT Initialize();

private:
    // Setup and shutdown

    HRESULT InitializeSapi();

    HRESULT InitializeTapi();

    void ShutdownSapi();

    void ShutdownTapi();

    // Main dialog proc is a friend function
    
    friend INT_PTR WINAPI MainDialogProc( HWND hDlg,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam );

    // TAPI actions
    
    HRESULT RegisterTapiEventInterface();
    
    HRESULT ListenOnAddresses();
    
    HRESULT ListenOnThisAddress( ITAddress * pAddress );
    
    HRESULT AnswerTheCall();
    
    HRESULT DisconnectTheCall();

    void ReleaseTheCall();

    HRESULT OnTapiEvent(
            TAPI_EVENT TapiEvent,
            IDispatch * pEvent );

    // SAPI actions

    HRESULT SetAudioOutForCall( ITLegacyCallMediaControl *pLegacyCallMediaControl );
    HRESULT SetAudioInForCall( ITLegacyCallMediaControl *pLegacyCallMediaControl );

    HRESULT HandleCall();

    // Miscellany

    void DoMessage( LPCWSTR pszMessage );

    void SetStatusMessage( LPCWSTR pszMessage );


private:
    // Win32 data
    HWND                    m_hDlg;
    BOOL                    m_fAutoAnswer;
    
    // TAPI data
    ITTAPI *                        m_pTapi;
    ITBasicCallControl *            m_pCall;
    CTAPIEventNotification *        m_pTAPIEventNotification;
    ULONG                           m_ulAdvise;

    // SAPI data

    // Audio
    CComPtr<ISpMMSysAudio>  m_cpMMSysAudioOut;
    CComPtr<ISpMMSysAudio>  m_cpMMSysAudioIn;

    // Text to speech
    CComPtr<ISpVoice>       m_cpLocalVoice;
    CComPtr<ISpVoice>       m_cpOutgoingVoice;

    // Speech Recognition
    CComPtr<ISpRecognizer>    m_cpIncomingRecognizer;
    CComPtr<ISpRecoContext>     m_cpIncomingRecoCtxt;
    CComPtr<ISpRecoGrammar>     m_cpDictGrammar;
};

// Helper function
BOOL AddressSupportsMediaType( ITAddress * pAddress,
                                long lMediaType );

