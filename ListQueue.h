// You may ask why bother writing own List? It's so basic data structure! Yeah! You;re right
// But structures in shitty std namespace are blowed up with unnecessary functionality.
// And if I use them this fucking CPP includes them in my code! It's not resolved by linking!
// I wanna keep my footprint low. OMG, CPP programmers, if you could imagine how C# is good!
// This is classic stuff for TDD, but in the shitty CPP there are no good tools for that. Sucks.
#pragma once
#include <stdlib.h>

class ListNode {
private:
	ListNode* next;

public:
	ListNode() {
		next = NULL;
	}

	// Oh yeah and no GC!
	~ListNode() {
		if (next != NULL) {
			delete next;
		}
	}

	ListNode* GetNext() {
		return next;
	}

	void SetNext(ListNode* next) {
		this->next = next;
	}
};

// It's a queue implemented as linked list
class ListQueue {
private:
	int count;
	ListNode* head;
	ListNode* tail;
	ListNode* current; // used for traversing the list

public:
	ListQueue() {
		current = head = tail = NULL;
		count = 0;
	}

	~ListQueue() {
		if (head != NULL) {
			delete head;
		}
	}

	void Enqueue(ListNode* node) {
		if (count == 0) {  
			head = tail = node;
			count = 1;
		}
		else {
			tail->SetNext(node);
			tail = node;
			count++;
		}
	}

	ListNode* Dequeue() {
		if (count == 0) {
			return NULL;
		}

		ListNode* oldHead = head;
		head = head->GetNext();
		count--;
		return oldHead;
	}

	int GetCount() {
		return count;
	}

	ListNode* GetNext() {
		if (current == NULL) {
			current = head;
		}
		else {
			current = current->GetNext();
		}

		return current;
	}

	void ResetIter() {
		current = NULL;
	}

	// The given node is previous node to that actually set for deletion
	// Doesn't check whether given node is in the list
	// If prevNode is NULL, head is assumed
	void Remove(ListNode* prevNode) {
		if (count == 0) throw "Attempt to remove from enpty list";
		ListNode* node;

		if (prevNode == NULL) {
			node = head;
			head = head->GetNext();
		} else {
			node = prevNode->GetNext();
			prevNode->SetNext(node->GetNext());
		}

		if (node == tail) {
			tail = prevNode;
		}

		node->SetNext(NULL); // needed so it wouldn't call delete for next node.
		delete node;
		count--;
	}
};