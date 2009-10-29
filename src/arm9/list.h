#ifndef _LIST_H_
#define _LIST_H_

#include <stdlib.h>

template <class C> class List {
   private:
   typedef struct Node {
      C elem;
      struct Node* next;
   } T_NODE;
   T_NODE* _list;
   T_NODE* _last;
   T_NODE* _actNode;
   int _actNodeIndex;
   int _anzNodes;
   int getElemIndex(C elem);

   public:
   List();
   virtual ~ List();

   virtual void appendElem(C elem);
   virtual void prependElem(C elem);
   virtual int getAnzElems();
   virtual C getElem(int index); // 1 .. getAnzElems
   virtual void deleteElem(C elem);
   virtual void deleteElemAtIndex(int index);
};

template <class C> List<C>::List()
{
   _list = NULL;
   _last = NULL;
   _actNode = NULL;
   _actNodeIndex = 0;
   _anzNodes = 0;
}

template <class C> List<C>::~ List()
{
   while (_list != NULL) {
      _actNode = _list->next;
      delete _list;
      _list = _actNode;
   }
}

template <class C> void List<C>::appendElem(C elem)
{
   _actNode = new T_NODE;
   _actNode->elem = elem;
   _actNode->next = NULL;
   _anzNodes++;
   _actNodeIndex = _anzNodes;
   if (_list == NULL) {
      _list = _actNode;
      _last = _actNode;
   }
   else {
      _last->next = _actNode;
      _last = _actNode;
   }
}

template <class C> void List<C>::prependElem(C elem)
{
   _actNode = new T_NODE;
   _actNode->elem = elem;
   _anzNodes++;
   _actNodeIndex = 1;
   if (_list == NULL) {
      _actNode->next = NULL;
      _list = _actNode;
      _last = _actNode;
   }
   else {
      _actNode->next = _list;
      _list = _actNode;
   }
}

template <class C> int List<C>::getAnzElems()
{
   return _anzNodes;
}

template <class C> C List<C>::getElem(int index)
{
   if (index > _anzNodes) {
      return NULL;
   }
   if (index == 1) {
      return _list->elem;
   }
   if (index == _anzNodes) {
      return _last->elem;
   }
   if (_actNodeIndex > index) {
      _actNode = _list;
      _actNodeIndex = 1;
   }
   while (_actNodeIndex < index) {
      _actNode = _actNode->next;
      _actNodeIndex++;
   }
   return _actNode->elem;
}

template <class C> int List<C>::getElemIndex(C elem)
{
   T_NODE* temp;

   if (_list == NULL) {
      return 0;
   }
   temp = _list;
   _actNode = _list;
   _actNodeIndex = 1;
   // handle view at first position
   if (temp->elem == elem) {
      return 1;
   }
   temp = temp->next; // set temp to actNode->next;
   while (temp != NULL) {
      if (temp->elem == elem) {
         return _actNodeIndex + 1;
      }
      _actNode = temp;
      _actNodeIndex++;
      temp = temp->next;
   }
   return 0;
}

template <class C> void List<C>::deleteElem(C elem)
{
   T_NODE* temp;
   int index;

   if (_list == NULL) {
      return;
   }
   index = getElemIndex(elem);
   if (index > 0) {
      _anzNodes--;
      if (index == 1) { // delete first element
         _list = _actNode->next;
         delete _actNode;
         _actNode = _list;
         if (_list == NULL) { // .. and only element
            _last = NULL;
            _actNodeIndex = 0;
         }
         else {
            _actNodeIndex = 1;
         }
      }
      else {
         temp = _actNode->next;
         if (temp->next == NULL) { // delete last element
            _last = _actNode;
         }
         _actNode->next = temp->next;
         delete temp;
      }
   }
}

template <class C> void List<C>::deleteElemAtIndex(int index)
{
   T_NODE* temp;

   if (index < 1) return;
   if (index > _anzNodes) return;
   _anzNodes--;
   temp = _list;
   if (index == 1) {
      _list = temp->next;
   }
   else {
      // point to index - 1
      while (index > 2) {
         temp = temp->next;
         index--;
      }
      _actNode = temp;
      temp = _actNode->next;
      _actNode->next = temp->next;
   }
   delete temp;
   _actNode = _list;
   _actNodeIndex = 1;
}

#endif // _LIST_H_
