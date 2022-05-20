

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 05:14:07 2038
 */
/* Compiler settings for Q2APO.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __Q2APO_h_h__
#define __Q2APO_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __IQ2APOMFX_FWD_DEFINED__
#define __IQ2APOMFX_FWD_DEFINED__
typedef interface IQ2APOMFX IQ2APOMFX;

#endif 	/* __IQ2APOMFX_FWD_DEFINED__ */


#ifndef __Q2APOMFX_FWD_DEFINED__
#define __Q2APOMFX_FWD_DEFINED__

#ifdef __cplusplus
typedef class Q2APOMFX Q2APOMFX;
#else
typedef struct Q2APOMFX Q2APOMFX;
#endif /* __cplusplus */

#endif 	/* __Q2APOMFX_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "audioenginebaseapo.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IQ2APOMFX_INTERFACE_DEFINED__
#define __IQ2APOMFX_INTERFACE_DEFINED__

/* interface IQ2APOMFX */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IQ2APOMFX;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("442A503D-5028-4061-A175-BDB83F811189")
    IQ2APOMFX : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IQ2APOMFXVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IQ2APOMFX * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IQ2APOMFX * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IQ2APOMFX * This);
        
        END_INTERFACE
    } IQ2APOMFXVtbl;

    interface IQ2APOMFX
    {
        CONST_VTBL struct IQ2APOMFXVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IQ2APOMFX_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IQ2APOMFX_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IQ2APOMFX_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IQ2APOMFX_INTERFACE_DEFINED__ */



#ifndef __Q2APOLib_LIBRARY_DEFINED__
#define __Q2APOLib_LIBRARY_DEFINED__

/* library Q2APOLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_Q2APOLib;

EXTERN_C const CLSID CLSID_Q2APOMFX;

#ifdef __cplusplus

class DECLSPEC_UUID("B551B56A-FB72-44D0-B545-C66911A8EFC8")
Q2APOMFX;
#endif
#endif /* __Q2APOLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


