//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File main.cc
// Date 05.11.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <QApplication>

#include <linslot.hpp>

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    LinslotWindow window;
    window.show();

    return app.exec();
}
