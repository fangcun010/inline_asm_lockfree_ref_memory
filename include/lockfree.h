/*
MIT License
Copyright(c) 2019 fangcun
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef _LOCKFREE_QUEUE_H_INCLUDED_
#define _LOCKFREE_QUEUE_H_INCLUDED_

#define			LOCKFREE_ALIGN_SIZE								8

extern "C" {
	struct lockfree_freelist_node;

	struct lockfree_freelist_pointer {
		lockfree_freelist_node *ptr;
		unsigned int tag;
	};

	struct lockfree_freelist_node { 
		lockfree_freelist_pointer next;
	};

	struct lockfree_freelist {
		lockfree_freelist_pointer top;
		unsigned int node_size;
	};

	struct lockfree_stack_node;

	struct lockfree_stack_pointer {
		lockfree_stack_node *ptr;
		unsigned int tag;
	};

	struct lockfree_stack_node {
		lockfree_stack_pointer next;
		void *value;
	};

	struct lockfree_stack {
		lockfree_stack_pointer top;
		lockfree_freelist *freelist;
	};

	struct lockfree_queue_node;

	struct lockfree_queue_pointer {
		lockfree_queue_node *ptr;
		unsigned int tag;
	};

	struct lockfree_queue_node {
		lockfree_queue_pointer next;
		void *value;
	};

	struct lockfree_queue {
		lockfree_queue_pointer head;
		lockfree_queue_pointer tail;
		lockfree_freelist *freelist;
	};

	struct lockfree_ref_memory;

	struct lockfree_ref_memory_node {
		lockfree_ref_memory *ref_memory;
		unsigned int ref_count;
		void *node;
	};

	struct lockfree_ref_memory_pointer {
		lockfree_ref_memory_node *node;
	};

	struct lockfree_ref_memory {
		void *free_function;
		lockfree_freelist *freelist;
	};

	lockfree_freelist * __cdecl lockfree_create_freelist(unsigned int node_size);
	void __cdecl lockfree_destroy_freelist(lockfree_freelist *freelist);
	void __cdecl lockfree_freelist_push(lockfree_freelist *freelist, void *node);
	void *__cdecl lockfree_freelist_pop(lockfree_freelist *freelist);
	void * __cdecl lockfree_freelist_alloc(lockfree_freelist *freelist);
	void __cdecl lockfree_freelist_free(lockfree_freelist *freelist, void *node);
	void __cdecl lockfree_freelist_clear(lockfree_freelist *freelist);
	int __cdecl lockfree_freelist_empty(lockfree_freelist *freelist);

	lockfree_stack * __cdecl lockfree_create_stack(lockfree_freelist *freelist);
	void __cdecl lockfree_destroy_stack(lockfree_stack *stack);
	void __cdecl lockfree_stack_push(lockfree_stack *stack, void *node);
	int __cdecl lockfree_stack_pop(lockfree_stack *stack,void *node);
	void __cdecl lockfree_stack_clear(lockfree_stack *stack);
	int __cdecl lockfree_stack_empty(lockfree_stack *stack);

	lockfree_queue * __cdecl lockfree_create_queue(lockfree_freelist *freelist);
	void __cdecl lockfree_destroy_queue(lockfree_queue *queue);
	void __cdecl lockfree_queue_push(lockfree_queue *queue, void *value);
	int __cdecl lockfree_queue_pop(lockfree_queue *queue, void *value);
	void __cdecl lockfree_queue_clear(lockfree_queue *queue);
	int __cdecl lockfree_queue_empty(lockfree_queue *queue);

	lockfree_ref_memory * __cdecl lockfree_create_ref_memory(lockfree_freelist *freelist, void *free_function);
	void __cdecl lockfree_destroy_ref_memory(lockfree_ref_memory *ref_memory);
	lockfree_ref_memory_pointer __cdecl lockfree_ref_memory_alloc(lockfree_ref_memory *ref_memory,void *node);
	void *__cdecl lockfree_ref_memory_get(lockfree_ref_memory_pointer ref_memory_pointer);
	void __cdecl lockfree_ref_memory_add_ref(lockfree_ref_memory_pointer ref_memory_pointer);
	void __cdecl lockfree_ref_memory_sub_ref(lockfree_ref_memory_pointer ref_memory_pointer);
}

#endif