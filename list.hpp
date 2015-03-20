//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File list.hpp
// Date 11.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//*************************************************************************
// Klasse List
//*************************************************************************

class List
{
   public:

      // Konstruktor & Destruktor

      enum Misc
      {
         success = 0,
         done    = success,
         fail    = -1,

         no      = 0,
         yes     = 1
      };

      List();
      ~List(); 

      // interface

      int insert(void *item);
      int append(void *item);
      int remove(void *item);
      int removeCurrent();

      int removeAll();
      int freeAll();

      // settings

      int setPosition(long i)   { Node* p = nodeAt(i); current = p ? p : current; return p ? success : fail; }

      // gettings      

      void* getFirst();
      void* getNext();
      void* getPrevious();
      void* getLast();
      void* getCurrent();
      void* getAt(int i)    { return nodeAt(i) ? nodeAt(i)->item : 0; }
      int getCount()        { return count; }
      
      int isEmpty()         { return count == 0; }

      int pushPosition();
      int popPosition();
      int removePushedPositions();

   protected:

      // Typ Node

      struct Node
      {
         void* item;

         Node* prev;
         Node* next;
      };

      int insertNode(Node* node, int after);
      int appendNode(Node* node);
      Node* nodeAt(int index);
   
      // register

      int count;
      Node* first;
      Node* last;
      Node* current;
      Node* pushedPos;
};
