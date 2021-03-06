// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright ? Microsoft Corporation. All rights reserved

/******************************************************************************
*           CoffeeShop4.cpp
*               Contains entry point for the CoffeeShop4 tutorial application, as
*               well as implementation of all application features.
*
*           The source code supplied here is intended as a sample, so some
*           error handling, etc. has been omitted for the sake of clarity.
******************************************************************************/
#include "stdafx.h"
#include <sphelper.h>                           // Contains definitions of SAPI functions
#include "common.h"                             // Contains common defines
#include "CoffeeShop4.h"                             // Forward declarations and constants
#include "cofgram.h"                            // This header is created by the grammar
                                                // compiler and has our rule ids

/******************************************************************************
* WinMain *
*---------*
*   Description:
*       CoffeeShop4 entry point.
*
*   Return:
*       exit code
******************************************************************************/
int APIENTRY WinMain(__in HINSTANCE hInstance,
                     __in_opt HINSTANCE hPrevInstance,
                     __in_opt LPSTR     lpCmdLine,
                     __in int       nCmdShow)
{
    MSG msg;

    // Register the main window class
    MyRegisterClass(hInstance, WndProc);

    // Initialize pane handler state
    g_fpCurrentPane = EntryPaneProc;

    // Only continue if COM is successfully initialized
    if ( SUCCEEDED( CoInitialize( NULL ) ) )
    {
        // Perform application initialization:
        if (!InitInstance( hInstance, nCmdShow )) 
        {
            return FALSE;
        }

        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        CoUninitialize();

        return (int)msg.wParam;
    }
    else
    {
        return 0;
    }
}

/******************************************************************************
* WndProc *
*---------*
*   Description:
*       Main window procedure.
*
******************************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
        case WM_CREATE:
            // Try to initialize, quit with error message if we can't
            if ( FAILED( InitSAPI( hWnd ) ) )
            {
                const int iMaxTitleLength = 64;
                TCHAR tszBuf[ MAX_PATH ];
                LoadString( g_hInst, IDS_FAILEDINIT, tszBuf, MAX_PATH );
                TCHAR tszName[ iMaxTitleLength ];
                LoadString( g_hInst, IDS_APP_TITLE, tszName, iMaxTitleLength );

                MessageBox( hWnd, tszBuf, tszName, MB_OK|MB_ICONWARNING );
                return( -1 );
            }
            // Let the entry pane know its time to do initialization work
            PostMessage( hWnd, WM_INITPANE, 0, 0 );
            break;
            
        // This is our application defined window message to let us know that a
        // speech recognition event has occurred.
        case WM_RECOEVENT:
            ProcessRecoEvent( hWnd );
            break;

        case WM_ERASEBKGND:
            EraseBackground( (HDC) wParam );
            return ( 1 );

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMM = (LPMINMAXINFO) lParam;

            lpMM->ptMaxSize.x = MINMAX_WIDTH;
            lpMM->ptMaxSize.y = MINMAX_HEIGHT;
            lpMM->ptMinTrackSize.x = MINMAX_WIDTH;
            lpMM->ptMinTrackSize.y = MINMAX_HEIGHT;
            lpMM->ptMaxTrackSize.x = MINMAX_WIDTH;
            lpMM->ptMaxTrackSize.y = MINMAX_HEIGHT;
            return ( 0 );
        }

        // Release remaining SAPI related COM references before application exits
        case WM_DESTROY:
            // Call the current pane's handler first
            (*g_fpCurrentPane)(hWnd, message, wParam, lParam);

            KillTimer( hWnd, 0 );
            CleanupGDIObjects();
            CleanupSAPI();
            PostQuitMessage(0);
            break;

        default:
        {
            _ASSERTE( g_fpCurrentPane );
            if ( g_fpCurrentPane == NULL)
            {
                return 0;
            }
            // Send unhandled messages to pane specific procedure for potential action
            LRESULT lRet = (*g_fpCurrentPane)(hWnd, message, wParam, lParam);
            if ( 0 == lRet )
            {
                lRet = DefWindowProc(hWnd, message, wParam, lParam);
            }
            return ( lRet );
        }
   }
   return ( 0 );
}

/******************************************************************************
* InitSAPI *
*----------*
*   Description:
*       Called once to get SAPI started.
*
******************************************************************************/
HRESULT InitSAPI( HWND hWnd )
{
    HRESULT hr = S_OK;

    while ( 1 )
    {
        // create a recognition engine
        hr = g_cpEngine.CoCreateInstance(CLSID_SpSharedRecognizer);
        if ( FAILED( hr ) )
        {
            break;
        }
       
        // create the command recognition context
        hr = g_cpEngine->CreateRecoContext( &g_cpRecoCtxt );
        if ( FAILED( hr ) )
        {
            break;
        }

        // Let SR know that window we want it to send event information to, and using
        // what message
        hr = g_cpRecoCtxt->SetNotifyWindowMessage( hWnd, WM_RECOEVENT, 0, 0 );
        if ( FAILED( hr ) )
        {
            break;
        }

        // Tell SR what types of events interest us.  Here we only care about command
        // recognition.
        hr = g_cpRecoCtxt->SetInterest( SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION) );
        if ( FAILED( hr ) )
        {
            break;
        }

        // Load our grammar, which is the compiled form of simple.xml bound into this executable as a
        // user defined ("SRGRAMMAR") resource type.
        hr = g_cpRecoCtxt->CreateGrammar(GRAMMARID1, &g_cpCmdGrammar);
        if (FAILED(hr))
        {
            break;
        }
        hr = g_cpCmdGrammar->LoadCmdFromResource(NULL, MAKEINTRESOURCEW(IDR_CMD_CFG),
                                                 L"SRGRAMMAR", MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                                 SPLO_DYNAMIC);
        if ( FAILED( hr ) )
        {
            break;
        }

        // Set navigation rule to active, espresso order rule to inactive
        hr = g_cpCmdGrammar->SetRuleIdState( VID_EspressoDrinks, SPRS_INACTIVE );
        if ( FAILED( hr ) )
        {
            break;
        }

        hr = g_cpCmdGrammar->SetRuleIdState( VID_Navigation, SPRS_ACTIVE );
        if ( FAILED( hr ) )
        {
            break;
        }

        // Get the default voice associated with our reco context
        hr = g_cpRecoCtxt->GetVoice(&g_cpVoice);
        if ( FAILED( hr ) )
        {
            break;
        }

        break;

    }

    // if we failed and have a partially setup SAPI, close it all down
    if ( FAILED( hr ) )
    {
        CleanupSAPI();
    }

    return ( hr );
}

/******************************************************************************
* CleanupSAPI *
*----------------*
*   Description:
*       Called to close down SAPI COM objects we have stored away.
*
******************************************************************************/
void CleanupSAPI( void )
{
    // Release grammar, if loaded
    if ( g_cpCmdGrammar )
    {
        g_cpCmdGrammar.Release();
    }
    // Release recognition context, if created
    if ( g_cpRecoCtxt )
    {
        g_cpRecoCtxt->SetNotifySink(NULL);
        g_cpRecoCtxt.Release();
    }
    // Release recognition engine instance, if created
    if ( g_cpEngine )
    {
        g_cpEngine.Release();
    }
    // Release voice, if created
    if ( g_cpVoice )
    {
        g_cpVoice.Release();
    }
}

/******************************************************************************
* ProcessRecoEvent *
*------------------*
*   Description:
*       Called to when reco event message is sent to main window procedure.
*       In the case of a recognition, it extracts result and calls ExecuteCommand.
*
******************************************************************************/
void ProcessRecoEvent( HWND hWnd )
{
    CSpEvent event;  // Event helper class

    // Loop processing events while there are any in the queue
    while (event.GetFrom(g_cpRecoCtxt) == S_OK)
    {
        // Look at recognition event only
        switch (event.eEventId)
        {
            case SPEI_RECOGNITION:
                ExecuteCommand(event.RecoResult(), hWnd);
                break;
            case SPEI_FALSE_RECOGNITION:
                HandleFalseReco(event.RecoResult(), hWnd);
                break;

        }
    }
}

/******************************************************************************
* ExecuteCommand *
*----------------*
*   Description:
*       Called to Execute commands that have been identified by the speech engine.
*
******************************************************************************/
void ExecuteCommand(ISpPhrase *pPhrase, HWND hWnd)
{
    SPPHRASE *pElements;

    // Get the phrase elements, one of which is the rule id we specified in
    // the grammar.  Switch on it to figure out which command was recognized.
    if (SUCCEEDED(pPhrase->GetPhrase(&pElements)))
    {        
        switch ( pElements->Rule.ulId )
        {
            case VID_EspressoDrinks:
            {
                ID_TEXT *pulIds = new ID_TEXT[MAX_ID_ARRAY];  // This memory will be freed when the WM_ESPRESSOORDER
                                                              // message is processed
                const SPPHRASEPROPERTY *pProp = NULL;
                const SPPHRASERULE *pRule = NULL;
                ULONG ulFirstElement, ulCountOfElements;
                int iCnt = 0;

                if ( pulIds )
                {
                    ZeroMemory( pulIds, sizeof( ID_TEXT[MAX_ID_ARRAY] ) );
                    pProp = pElements->pProperties;
                    pRule = pElements->Rule.pFirstChild;
                    // Fill in an array with the drink properties received
                    while ( pProp && iCnt < MAX_ID_ARRAY )
                    {
                        // Fill out a structure with all the property ids received as well
                        // as their corresponding text
                        pulIds[iCnt].ulId = static_cast< ULONG >(pProp->pFirstChild->vValue.ulVal);
                        // Get the count of elements from the rule ref, not the actual leaf
                        // property
                        if ( pRule )
                        {
                            ulFirstElement = pRule->ulFirstElement;
                            ulCountOfElements = pRule->ulCountOfElements;
                        }
                        else
                        {
                            ulFirstElement = 0;
                            ulCountOfElements = 0;
                        }
                        // This is the text corresponding to property iCnt - it must be
                        // released when we are done with it
                        pPhrase->GetText( ulFirstElement, ulCountOfElements,
                                          FALSE, &(pulIds[iCnt].pwstrCoMemText), NULL);
                        // Loop through all properties
                        pProp = pProp->pNextSibling;
                        // Loop through rulerefs corresponding to properties
                        if ( pRule )
                        {
                            pRule = pRule->pNextSibling;
                        }
                        iCnt++;
                    }
                    PostMessage( hWnd, WM_ESPRESSOORDER, NULL, (LPARAM) pulIds );                   
                }
            }
            break;
            case VID_Navigation:
            {
                switch( pElements->pProperties->vValue.ulVal )
                {
                    case VID_Counter:
                        PostMessage( hWnd, WM_GOTOCOUNTER, NULL, NULL );                        
                    break;
                    case VID_Office:
                        PostMessage( hWnd, WM_GOTOOFFICE, NULL, NULL );                        
                    break;
                }
            }
            break;

            case VID_Manage:
            {
                switch( pElements->pProperties->vValue.ulVal )
                {
                    case VID_Employees:
                        PostMessage( hWnd, WM_MANAGEEMPLOYEES, NULL, NULL );                        
                    break;
                }
            }
            break;

            case VID_HearTheVoice:
            {
                PostMessage( hWnd, WM_HEARTHEVOICE, NULL, NULL );                                        
            }
            break;

        }
        // Free the pElements memory which was allocated for us
        ::CoTaskMemFree(pElements);
    }

}

/******************************************************************************
* EntryPaneProc *
*---------------*
*   Description:
*       Handles messages specifically for the entry pane.
*
******************************************************************************/
LRESULT EntryPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
        case WM_INITPANE:
        {
            TCHAR tBuf[MAX_LOADSTRING];

            LoadString( g_hInst, IDS_WELCOME, tBuf, MAX_LOADSTRING );
            // Speak the welcome prompt, and do not wait for the operation to complete
            g_cpVoice->Speak( CT2W(tBuf), SPF_ASYNC, NULL);
            return ( 1 );
        }

        case WM_GOTOCOUNTER:
            // Set the right message handler and repaint
            g_fpCurrentPane = CounterPaneProc;
            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );

        case WM_GOTOOFFICE:
            // Set the right message handler and repaint
            g_fpCurrentPane = OfficePaneProc;
            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );

        case WM_PAINT:
            EntryPanePaint( hWnd );
            return ( 1 );
    }
    return ( 0 );

}

/******************************************************************************
* CounterPaneProc *
*-----------------*
*   Description:
*       Handles messages specifically for the counter (order) pane.
*
******************************************************************************/
LRESULT CounterPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    HRESULT hr;

    switch ( message )
    {
        case WM_ESPRESSOORDER:
        {
            ID_TEXT *pulIds = (ID_TEXT *) lParam;
            _ASSERTE( pulIds );
            if ( pulIds == NULL )
            {
                return 0;
            }
            KillTimer( hWnd, 0 );
            int i = 0, ilen = 0;
            TCHAR szTempBuf[NORMAL_LOADSTRING];
            TCHAR szSpace[] =  _T(" ");
            int iTemplen;
            
            g_szCounterDisplay[0] = '\0';

            // Sort the array
            while ( 0 != pulIds[i].ulId )
            {
                i++;
            }
            for ( int j = 0; j < i; j++ )
            {
                int iminIndex = j;               
                for ( int k = j; k < i; k++ )
                {
                    if ( pulIds[iminIndex].ulId > pulIds[k].ulId )
                    {
                        iminIndex = k;
                    }
                }
                ULONG ulId = pulIds[iminIndex].ulId;
                WCHAR *pwstr = pulIds[iminIndex].pwstrCoMemText;
                pulIds[iminIndex].pwstrCoMemText = pulIds[j].pwstrCoMemText;
                pulIds[j].pwstrCoMemText = pwstr;
                pulIds[iminIndex].ulId = pulIds[j].ulId;
                pulIds[j].ulId = ulId;
            }
            
            i = 0;
            // Put in the first order words if we actually have an order
            if ( 0 != pulIds[0].ulId )
            {
                iTemplen = LoadString( g_hInst, IDS_ORDERBEGIN, szTempBuf, NORMAL_LOADSTRING );                
                _tcscat_s( g_szCounterDisplay + ilen, _countof(g_szCounterDisplay) - ilen, szTempBuf );
                ilen += iTemplen;
            }
            while ( i < MAX_ID_ARRAY && 0 != pulIds[i].ulId )
            {
                CW2T pTempStr( pulIds[i].pwstrCoMemText );

                iTemplen = lstrlen( pTempStr );
                // We'll quit now so we dont overrun the buffer
                if ( ilen + iTemplen >= MAX_LOADSTRING )
                {
                    break;
                }
                if ( i > 0 )
                {
                    _tcscat_s( g_szCounterDisplay + ilen, _countof(g_szCounterDisplay) - ilen, szSpace );
                    ilen += 1;
                }
                _tcscat_s( g_szCounterDisplay, _countof(g_szCounterDisplay), pTempStr );
                ilen += iTemplen;
                i++;
            }
            // Put the thank you on this order
            if ( 0 < i )
            {
                iTemplen = LoadString( g_hInst, IDS_ORDEREND, szTempBuf, NORMAL_LOADSTRING );                
                if ( ilen + iTemplen < MAX_LOADSTRING )
                {
                    _tcscat_s( g_szCounterDisplay + ilen, _countof(g_szCounterDisplay) - ilen, szTempBuf );
                    ilen += iTemplen;
                }
            }

            InvalidateRect( hWnd, NULL, TRUE );
            SetTimer( hWnd, 0, TIMEOUT, NULL );

            // Speak the order
            g_cpVoice->Speak( CT2W(g_szCounterDisplay), SPF_ASYNC, NULL);

            // Delete the CoTaskMem we were given initially by ISpPhrase->GetText
            i = 0;
            while ( i < MAX_ID_ARRAY && 0 != pulIds[i].ulId )
            {
                CoTaskMemFree( pulIds[i].pwstrCoMemText );
                i++;
            }
            delete [] pulIds;
            return ( 1 );

        }

        case WM_PAINT:
            CounterPanePaint( hWnd, g_szCounterDisplay );
            return ( 1 );

        case WM_INITPANE:
            LoadString( g_hInst, IDS_PLEASEORDER, g_szCounterDisplay, MAX_LOADSTRING );
            // Set the rule recognizing an espresso order to active, now that we are ready for it
            g_cpCmdGrammar->SetRuleIdState( VID_EspressoDrinks, SPRS_ACTIVE );

            // Set our interests to include false recognitions
            hr = g_cpRecoCtxt->SetInterest( SPFEI(SPEI_RECOGNITION)|SPFEI(SPEI_FALSE_RECOGNITION),
                                                    SPFEI(SPEI_RECOGNITION)|SPFEI(SPEI_FALSE_RECOGNITION) );
            _ASSERTE( SUCCEEDED( hr ) );

            // Speak the welcome string
            g_cpVoice->Speak( CT2W(g_szCounterDisplay), SPF_ASYNC, NULL);

            return ( 1 );

        case WM_TIMER:
            // Revert back to 'go ahead and order' message
            LoadString( g_hInst, IDS_PLEASEORDER, g_szCounterDisplay, MAX_LOADSTRING );
            InvalidateRect( hWnd, NULL, TRUE );

            // Speak the welcome string
            g_cpVoice->Speak( CT2W(g_szCounterDisplay), SPF_ASYNC, NULL);

            KillTimer( hWnd, 0 );
            return ( 1 );

        case WM_GOTOOFFICE:
            KillTimer( hWnd, 0 );
            // Set the rule recognizing an espresso order to inactive
            // since you cant order from the office
            g_cpCmdGrammar->SetRuleIdState( VID_EspressoDrinks, SPRS_INACTIVE );

            // Set our interests to include only recognitions
            hr = g_cpRecoCtxt->SetInterest( SPFEI(SPEI_RECOGNITION),SPFEI(SPEI_RECOGNITION) );
            _ASSERTE( SUCCEEDED( hr ) );

            // Set the right message handler and repaint
            g_fpCurrentPane = OfficePaneProc;
            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );

        case WM_DIDNTUNDERSTAND:
            KillTimer( hWnd, 0 );
            LoadString( g_hInst, IDS_DIDNTUNDERSTAND, g_szCounterDisplay, MAX_LOADSTRING );
            InvalidateRect( hWnd, NULL, TRUE );
            // Speak the didn't understand string
            g_cpVoice->Speak( CT2W(g_szCounterDisplay), SPF_ASYNC, NULL);
            SetTimer( hWnd, 0, TIMEOUT, NULL );
            return ( 1 );

    }
    return ( 0 );
}

/******************************************************************************
* OfficePaneProc *
*---------------*
*   Description:
*       Handles messages specifically for the office pane.
*
******************************************************************************/
LRESULT OfficePaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    HRESULT hr;
    
    switch ( message )
    {
        case WM_GOTOCOUNTER:
            // Set management rule to inactive
            hr = g_cpCmdGrammar->SetRuleIdState( VID_Manage, SPRS_INACTIVE );            
            _ASSERTE( SUCCEEDED( hr ) );

            // Set the right message handler and repaint
            g_fpCurrentPane = CounterPaneProc;
            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );

        case WM_MANAGEEMPLOYEES:
            // Set management rule to inactive
            hr = g_cpCmdGrammar->SetRuleIdState( VID_Manage, SPRS_INACTIVE );            
            _ASSERTE( SUCCEEDED( hr ) );

            // Set the right message handler and repaint
            g_fpCurrentPane = ManageEmployeesPaneProc;
            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );

        case WM_PAINT:
            OfficePanePaint( hWnd );
            return ( 1 );

        case WM_INITPANE:
            // Set management rule to active
            hr = g_cpCmdGrammar->SetRuleIdState( VID_Manage, SPRS_ACTIVE );            
            _ASSERTE( SUCCEEDED( hr ) );
            return ( 1 );

    }
    return ( 0 );
}

/******************************************************************************
* HandleFalseReco *
*----------------*
*   Description:
*       Called to respond to false recognition events.
*
******************************************************************************/
void HandleFalseReco(ISpRecoResult *pRecoResult, HWND hWnd)
{
    SPRECORESULTTIMES resultTimes;

    if (SUCCEEDED( pRecoResult->GetResultTimes( &resultTimes ) ) )
    {
        if ( GetTickCount() - resultTimes.dwTickCount > MIN_ORDER_INTERVAL )
        {
            PostMessage( hWnd, WM_DIDNTUNDERSTAND, 0, 0);
        }
    }
}

/******************************************************************************
* ManageEmployeesPaneProc *
*-------------------------*
*   Description:
*       Handles messages specifically for the manage employees pane.
*
******************************************************************************/
LRESULT ManageEmployeesPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    static ULONG ulNumTokens;
    static ULONG ulCurToken;
    static WCHAR**  ppszTokenIds;
    static CSpDynamicString*  ppcDescriptionString;     // This is string helper class in sphelper.h
    
    switch ( message )
    {
        case WM_GOTOOFFICE:
        {
            // Set the right message handler and repaint
            g_fpCurrentPane = OfficePaneProc;
            //Cleanup our variables
            ManageEmployeesPaneCleanup( ppszTokenIds, ppcDescriptionString, ulNumTokens );            
            ppszTokenIds = NULL;
            ppcDescriptionString = NULL;
            ulNumTokens = 0;

            // Set the hear voice rule to inactive
            HRESULT hr = g_cpCmdGrammar->SetRuleIdState( VID_HearTheVoice, SPRS_INACTIVE );            
            _ASSERTE( SUCCEEDED( hr ) );

            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );
        }

        case WM_GOTOCOUNTER:
        {
            // Set the right message handler and repaint
            g_fpCurrentPane = CounterPaneProc;
            //Cleanup our variables
            ManageEmployeesPaneCleanup( ppszTokenIds, ppcDescriptionString, ulNumTokens );            
            ppszTokenIds = NULL;
            ppcDescriptionString = NULL;
            ulNumTokens = 0;

            // Set the hear voice rule to inactive
            HRESULT hr = g_cpCmdGrammar->SetRuleIdState( VID_HearTheVoice, SPRS_INACTIVE );            
            _ASSERTE( SUCCEEDED( hr ) );

            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );
        }
        case WM_PAINT:
        {
            // Do the actual UI paint
            ManageEmployeesPanePaint( hWnd, ulNumTokens, ppcDescriptionString, ulCurToken );
            return ( 1 );
        }

        case WM_INITPANE:
        {
            ISpObjectToken                  *pToken = NULL;  // Token interface pointer
            CComPtr<IEnumSpObjectTokens>    cpEnum;          // Pointer to token enumerator
            ULONG                           ulIndex = 0;
            ulCurToken = 0xffffffff;

            // Get a token enumerator for tts voices available
            HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);
            if ( S_OK == hr )
            {
                // Get the numbers of tokens found
                hr = cpEnum->GetCount( &ulNumTokens );

                if ( SUCCEEDED( hr ) && 0 != ulNumTokens )
                {
                    // Create arrays we need for storing data
                    ppcDescriptionString = new CSpDynamicString [ulNumTokens];
                    if ( NULL == ppcDescriptionString )
                    {
                        break;
                    }

                    ppszTokenIds = new WCHAR* [ulNumTokens];
                    if ( NULL == ppszTokenIds )
                    {
                        break;
                    }
                    ZeroMemory( ppszTokenIds, ulNumTokens*sizeof( WCHAR* ) );                    
                    
                    // Get the next token in the enumeration
                    // State is maintained in the enumerator
                    while (cpEnum->Next(1, &pToken, NULL) == S_OK)
                    {
                        // Get a string which describes the token, in our case, the voice name
                        hr = SpGetDescription( pToken, &ppcDescriptionString[ulIndex] );
                        _ASSERTE( SUCCEEDED( hr ) );
                        
                        // Get the token id, for a low overhead way to retrieve the token later
                        // without holding on to the object itself
                        hr = pToken->GetId( &ppszTokenIds[ulIndex] );
                        _ASSERTE( SUCCEEDED( hr ) );
                        
                        ulIndex++;
                        
                        // Release the token itself
                        pToken->Release();
                        pToken = NULL;
                    }                   
                }
                
                // if we've failed to properly initialize, then we should completely shut-down
                if ( S_OK != hr )
                {
                    if ( pToken )
                    {
                        pToken->Release();
                    }
                    ManageEmployeesPaneCleanup( ppszTokenIds, ppcDescriptionString, ulNumTokens );

                    ppszTokenIds = NULL;
                    ppcDescriptionString = NULL;
                    ulNumTokens = 0;
                }
                // Find out which token corresponds to our voice which is currently in use
                else
                {
                    WCHAR *pszCurTokenId = NULL;

                    // Get the token representing the current voice
                    hr = g_cpVoice->GetVoice( &pToken );
                    if ( SUCCEEDED( hr ) )
                    {
                        // Get the current token ID, and compare it against others to figure out
                        // which description string is the one currently selected.
                        hr = pToken->GetId( &pszCurTokenId );
                        if ( SUCCEEDED( hr ) )
                        {
                            ulIndex = 0;
                            while ( ulIndex < ulNumTokens && 
                                    0 != _wcsicmp( pszCurTokenId, ppszTokenIds[ulIndex] ) )
                            {
                                ulIndex++;
                            }

                            // We found it, so set the current index to that of the current token
                            if ( ulIndex < ulNumTokens )
                            {
                                ulCurToken = ulIndex;
                            }

                            CoTaskMemFree( pszCurTokenId );
                        }

                        pToken->Release();

                    }                                       

                }
            
            }

            // Set the hear voice rule to active
            hr = g_cpCmdGrammar->SetRuleIdState( VID_HearTheVoice, SPRS_ACTIVE );            
            _ASSERTE( SUCCEEDED( hr ) );

            return ( 1 );
        }

        case WM_DESTROY:
            // Windows is closing down, so we should cleanup
            ManageEmployeesPaneCleanup( ppszTokenIds, ppcDescriptionString, ulNumTokens );
            ppszTokenIds = NULL;
            ppcDescriptionString = NULL;
            ulNumTokens = 0;
            return ( 1 );

        case WM_HEARTHEVOICE:
            // Set the voice to play
            LoadString( g_hInst, IDS_VOICESPEAK, g_szCounterDisplay, MAX_LOADSTRING );
            g_cpVoice->Speak( CT2W(g_szCounterDisplay), SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL );
    }
    return ( 0 );
}

/******************************************************************************
* ManageEmployeesPaneCleanup *
*----------------------------*
*   Description:
*       Helper function to cleanup objects allocated while the ManageEmployees
*       pane is running.
*
******************************************************************************/
void ManageEmployeesPaneCleanup( __in_ecount_opt(ulNumTokens) WCHAR** ppszTokenIds, 
                                 CSpDynamicString*  ppcDescriptionString, ULONG ulNumTokens)
{
    ULONG ulIndex;

    // Free all allocated token ids
    if ( ppszTokenIds )
    {
        for ( ulIndex = 0; ulIndex < ulNumTokens; ulIndex++ )
        {
            CoTaskMemFree( ppszTokenIds[ulIndex] );
        }
        
        delete [] ppszTokenIds;
    }
    
    delete [] ppcDescriptionString;
}
