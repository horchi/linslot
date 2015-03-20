//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File linslot.hpp
// Date 11.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#include <stdio.h>
#include <malloc.h>

#include <list.hpp>

//*************************************************************************
// Klasse List
//*************************************************************************
//*************************************************************************
// Konstruktor
//*************************************************************************

List::List()
{
   count = 0;
   first = last = current = 0;
   pushedPos = 0;
}

//*************************************************************************
// Destruktor
//*************************************************************************

List::~List()
{
   if (!count)
      return;

   removeAll();
}

//*************************************************************************
// Interface
//*************************************************************************
//*************************************************************************
// Einfuegen in die Liste vor der aktuellen Position (Zeiger current)
//*************************************************************************

int List::insert(void* item)
{
   Node* tmp;
   
   // valid item ?

   if (!item)
      return fail;

   // get new node and store in list
   
   tmp = new Node;
   tmp->item = item;
   
   if (!count || !current)
      appendNode(tmp);
   else
      insertNode(tmp, /*after = */no); // insert *before* current position !!

   return success;
}

//***************************************************************************
// Insert Node
//***************************************************************************

int List::insertNode(Node* node, int after)
{
   if (after)
   {
      // insert node *after* current position !!

      node->next = current->next;
      node->prev = current;

      node->next->prev = node;
      node->prev->next = node;

      if (current == last)
         last = node;
   }
   else
   {
      // insert node before current position !!

      node->next = current;
      node->prev = current->prev;

      node->next->prev = node;
      node->prev->next = node;

      if (current == first)
         first = node;
   }

   count++;
   current = node;

   return done;
}
//*************************************************************************
// Append Node
//*************************************************************************

int List::appendNode(Node* node)
{
   // first item ?

   if (!count)
      first = last = node;
   
   node->prev = last;
   node->next = first;

   last->next  = node;
   last        = node;
   first->prev = node;

   count++;

   return success;
}

//*************************************************************************
// Anhaengen eines Elements an die letzte Position
//*************************************************************************

int List::append(void* item)
{
   Node* tmp;
   
   if (!item)
      return fail;
   
   tmp = new Node;
   tmp->item = item;

   // first item ?

   appendNode(tmp);

   return success;
}

//***************************************************************************
// Remove Current Item
//***************************************************************************

int List::removeCurrent()
{
   Node* tmp = current;

   if (!current || !count)
      return fail;

   // current auf Element *vor* dem entfernten positionieren !

   if (current != first)
      current = tmp->prev;
   else
      current = 0;

   tmp->prev->next = tmp->next;
   tmp->next->prev = tmp->prev;
   
   count--;

   // check first/last pointer

   if (!count)
      current = first = last = 0;
   else if (tmp == first)
      first = tmp->next;
   else if (tmp == last)
      last = tmp->prev;
   
   delete tmp;

   return success;
}

//*************************************************************************
// Entfernen eines bestimmten Elementes aus der Liste
//*************************************************************************

int List::remove(void* item)
{
   Node* tmp;

   if (!count)
      return fail;

   if (!item)
      return fail;

   // search item

   for (tmp = first; (tmp->item != item) && (tmp != last); tmp = tmp->next) { } 

   // found ?

   if (tmp->item != item)
      return fail;

   if (--count)
   {
      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
  
      // check first/last pointer

      if (tmp == first)
         first = tmp->next;
      else if (tmp == last)
         last  = tmp->prev;
   }
   else
   {
      first = last = 0;
   }

   // clear current

   current = 0;

   // clear item

   delete tmp;

   return success;
}

//*************************************************************************
// Remove all items
//*************************************************************************

int List::removeAll()
{
   Node *next, *tmp = first;

   // clear all items

   for (int i = 0; i < count; i++)
   {
      // got next item

      next = tmp->next;

      delete tmp;
   
      // gehe zum nachsten

      tmp = next;
   }

   // reset counter

   count = 0;

   removePushedPositions();

   return success;
}

//*************************************************************************
// Remove and delete all items
//*************************************************************************

int List::freeAll()
{
   Node *next, *tmp = first;

   for (int i = 0; i < count; i++)
   {
      // remove item buffer

      free(tmp->item);

      // goto next item

      next = tmp->next;

      delete tmp;
      
      tmp = next;
   }

   count = 0;

   return success;
}

//*************************************************************************
// Gettings
//*************************************************************************
//*************************************************************************
// Hole erstes Element
//*************************************************************************

void* List::getFirst()
{
   if (!count)
      return 0;

   // init cuurent

   current = first;

   return current->item;
}

//*************************************************************************
// Hole letztes Element
//*************************************************************************

void* List::getLast()
{
   if (!count)
      return 0;

   // init current

   current = last;

   return current->item;
} 

//*************************************************************************
// Hole naechstes Element
//*************************************************************************

void* List::getNext()
{
   if (!count)
      return 0;

   if (!current)
      return getFirst();

   if (current == last)
      return current = 0;

   // goto next item

   current = current->next;

   return current->item;
}

//*************************************************************************
// Hole vorheriges Element
//*************************************************************************

void* List::getPrevious()
{
   if (!count)
      return 0;

   if (!current)
      return getLast();

   if (current == first)
      return current = 0;

   // goto previous item

   current = current->prev;

   return current->item;
}

//***************************************************************************
// Push Position
//***************************************************************************

int List::pushPosition()
{
   Node* newNode = new Node;

   newNode->item = current;
   newNode->prev = 0;
   newNode->next = 0;

   if (pushedPos)
   {
      pushedPos->next = newNode;
      newNode->prev = pushedPos;
   }

   pushedPos = newNode;

  return done;
}

//***************************************************************************
// Pop Position
//***************************************************************************

int List::popPosition()
{
   Node* prevPos;

   if (!pushedPos)
   {
      current = 0;
      return fail;
   }

   prevPos = pushedPos->prev;

   current = (Node*) pushedPos->item;

   pushedPos->item = 0;
   pushedPos->prev = 0;
   pushedPos->next = 0;
   
   delete pushedPos;

   pushedPos = prevPos;

   if (pushedPos)
      pushedPos->next = 0;

   return done;
}

//*************************************************************************
// Hole aktuelles Element
//*************************************************************************

void* List::getCurrent()
{
   return current ? current->item : 0;
}

//*************************************************************************
// bestimmtes Elemente liefern,
// dieses wird jedoch nicht zum aktuellen
//*************************************************************************

List::Node* List::nodeAt(int index)
{
   Node* p = first;  // der Anfang

   // Liste leer ?

   if (!count) return 0;

   // Index gueltig ?

   if (index >= count || index < 0)  return 0;

   for (int i = 0; i < index; i++)
      p = p->next;

   return p;
}

//*************************************************************************
// Remove all pushed positions
//*************************************************************************

int List::removePushedPositions()
{
   Node* prev = 0;
   Node* iter = 0;

   if (!pushedPos)
      return success;

   iter = pushedPos;

   while (iter)
   {
      prev = iter->prev;
      iter->item = 0;
      iter->next = 0;
      iter->prev = 0;
      delete iter;
      iter = prev;
   }

   pushedPos = 0;

   return success;
}
