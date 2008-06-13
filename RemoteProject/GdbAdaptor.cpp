
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "GdbAdaptor.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

CGdbAdaptor::CGdbAdaptor()
{
}

CGdbAdaptor::~CGdbAdaptor()
{
}

void CGdbAdaptor::Init(CRemoteProject* /*pProject*/)
{
}

CString CGdbAdaptor::TransformInput(LPCTSTR pstrInput)
{
   if( *pstrInput != '-' ) return pstrInput;

   CString sCommand;
   sCommand.Format(_T("232%s"), pstrInput);

   // FIX: Argh, GDB MI doesn't come with a proper "SetNextStatement"
   //      command. The best we can do is to try to jump in the local file!
   if( sCommand.Find(_T("232-exec-jump")) == 0 ) {
      int iPos = sCommand.Find(':');
      if( iPos > 0 ) sCommand.Format(_T("232-interpreter-exec console \"jump %ld\""), _ttol(sCommand.Mid(iPos + 1)));
   }

   return sCommand;
}

void CGdbAdaptor::TransformOutput(LPCTSTR pstrOutput, CSimpleArray<CString>& aOutput)
{
   CString str = pstrOutput;   // Boring...
   aOutput.Add(str);
}

