//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File iointerface.cc
// Date 19.01.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Includes
//***************************************************************************

#include <iointerface.hpp>
 
//***************************************************************************
// Object
//***************************************************************************

IoInterface::IoInterface()
{
   *deviceName = 0;
   SlotService::tvNow(&tvBoardStartTime);
}

IoInterface::~IoInterface()
{
}
