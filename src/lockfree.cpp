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
#include <stdlib.h>
#include <malloc.h>
#include <lockfree.h>

extern "C" {
	lockfree_freelist * __cdecl lockfree_create_freelist(
		unsigned int node_size) {
		__asm {
			push LOCKFREE_ALIGN_SIZE
			push TYPE lockfree_freelist
			call _aligned_malloc
			add esp, 8

			cmp eax, 0

			jne alloc_malloc_ok

			jmp exit_function

			alloc_malloc_ok :
			mov ebx, 0
				mov[eax], ebx
				mov [eax+4],ebx
				mov ebx, node_size
				mov[eax + 8], ebx

				exit_function :
		}
	}

	void __cdecl lockfree_destroy_freelist(lockfree_freelist *freelist) {
		__asm {
			push freelist
			call lockfree_freelist_clear
			add esp, 4

			push freelist
			call _aligned_free
			add esp, 4
		}
	}

	void __cdecl lockfree_freelist_push(lockfree_freelist *freelist, void *node) {
#define			register_freelist			mm5
#define			register_node				mm6
#define			register_top				mm7
		__asm {
			movd register_freelist, freelist
			movd register_node,node

			cas_loop_start:

			movd ebx,register_freelist
			movq register_top,[ebx]

			movd ebx,register_node
			movq mm0,register_top
			movd eax,mm0
			mov[ebx],eax
			psrlq mm0,32
			movd eax,mm0
			mov [ebx+4],eax

			movd edi,register_freelist
			movq mm0,register_top
			movd eax,mm0
			psrlq mm0,32
			movd edx,mm0
			movd ebx,register_node
			mov ecx,edx
			inc ecx

			lock cmpxchg8b [edi]

			jnz cas_loop_start
			emms
		}
#undef			register_freelist
#undef			register_node
#undef			register_top
	}

	void *__cdecl lockfree_freelist_pop(lockfree_freelist *freelist) {
#define			register_freelist			mm5
#define			register_next				mm6
#define			register_top				mm7
		__asm {
			movd register_freelist, freelist

			cas_loop_start:

			movd ebx,register_freelist
			movq register_top,[ebx]

			movd eax,register_top
			cmp eax, 0
			je exit_function

			movq register_next,[eax]

			movd edi,register_freelist

			movq mm0, register_top
			movd eax, mm0
			psrlq mm0, 32
			movd edx, mm0
			movd ebx, register_next
			mov ecx, edx
			inc ecx

			lock cmpxchg8b[edi]
			jnz cas_loop_start
			movd eax,register_top
			exit_function :
			emms
		}
#undef			register_freelist
#undef			register_next
#undef			register_top
	}

	void * __cdecl lockfree_freelist_alloc(lockfree_freelist *freelist) {
		__asm {
			push freelist
			call lockfree_freelist_pop
			add esp, 4
			cmp eax, 0
			jne exit_function
			mov eax, freelist

			mov ebx, [eax + 8]
			add ebx, 8
			push LOCKFREE_ALIGN_SIZE
			push ebx
			call _aligned_malloc
			add esp, 8
			cmp eax, 0
			jne exit_function
			call abort
			exit_function :

		}
	}

	void __cdecl lockfree_freelist_free(lockfree_freelist *freelist, void *node) {
		__asm {
			push node
			push freelist
			call lockfree_freelist_push
			add esp, 8
		}
	}

	void __cdecl lockfree_freelist_clear(lockfree_freelist *freelist) {
		__asm {
		clear_loop_start:
			push freelist
				call lockfree_freelist_pop
				add esp, 4
				cmp eax, 0
				je exit_function
				push eax
				call _aligned_free
				add esp, 4
				jmp clear_loop_start
				exit_function :
		}
	}

	int __cdecl lockfree_freelist_empty(lockfree_freelist *freelist) {
		__asm {
			mov ebx, freelist

			mov eax, [ebx]
			cmp eax, 0
			je return_true
			mov eax, 0
			jmp exit_function
			return_true :
			mov eax, 1
				exit_function :
		}
	}

	lockfree_stack * __cdecl lockfree_create_stack(lockfree_freelist *freelist) {
		__asm {
			push LOCKFREE_ALIGN_SIZE
			push TYPE lockfree_stack
			call _aligned_malloc
			add esp, 8

			cmp eax, 0

			jne alloc_malloc_ok

			jmp exit_function

			alloc_malloc_ok :
			mov ebx, 0
			mov[eax], ebx
			mov[eax +4], ebx
			mov ebx, freelist
			mov[eax + 8], ebx

			exit_function :
		}
	}

	void __cdecl lockfree_destroy_stack(lockfree_stack *stack) {
		__asm {
			push stack
			call lockfree_stack_clear
			add esp, 4

			push stack
			call _aligned_free
			add esp, 4
		}
	}

	void __cdecl lockfree_stack_push(lockfree_stack *stack, void *node) {
#define			register_stack				mm5
#define			register_node				mm6
#define			register_top				mm7
		__asm {
			mov ebx, stack
			mov eax, [ebx + 8]
			mov ecx, [eax + 8]

			push ecx

			push eax

			call lockfree_freelist_alloc
			add esp,4

			pop ecx

			cmp eax,0
			jne alloc_node_ok

			call abort

			alloc_node_ok:

			mov esi,node
			mov edi,eax
			add edi,8

			rep movsb

			movd register_stack, stack
			movd register_node, eax

			cas_loop_start :

			movd ebx, register_stack
				movq register_top, [ebx]

				movd ebx, register_node
				movq mm0, register_top
				movd eax, mm0
				mov[ebx], eax
				psrlq mm0, 32
				movd eax, mm0
				mov[ebx + 4], eax

				movd edi, register_stack
				movq mm0, register_top
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				movd ebx, register_node
				mov ecx, edx
				inc ecx

				lock cmpxchg8b[edi]

				jnz cas_loop_start
				emms
		}
#undef			register_stack
#undef			register_node
#undef			register_top
	}

	int __cdecl lockfree_stack_pop(lockfree_stack *stack,void *node) {
#define			register_stack				mm4
#define			register_next				mm6
#define			register_top				mm7
		__asm {
			movd register_stack, stack

			cas_loop_start :

			movd ebx, register_stack
				movq register_top, [ebx]

				movd eax, register_top
				cmp eax, 0
				jne not_empty
				emms
				jmp exit_function

				not_empty:

				movq register_next, [eax]

				movd edi, register_stack

				movq mm0, register_top
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				movd ebx, register_next
				mov ecx, edx
				inc ecx

				lock cmpxchg8b[edi]
				jnz cas_loop_start
				
				movd edx,register_top
				mov esi, edx
				add esi,8

				movd eax,register_stack
				mov ebx,[eax+8]
				mov ecx,[ebx+8]

				mov edi,node

				rep movsb

				emms

				push edx
				push ebx

				call lockfree_freelist_push

				add esp,8

				mov eax,1

				exit_function :
		}
#undef			register_stack
#undef			register_next
#undef			register_top
	}

	void __cdecl lockfree_stack_clear(lockfree_stack *stack) {
		unsigned int node_size = stack->freelist->node_size;
		unsigned char *node = (unsigned char *)_aligned_malloc(node_size, LOCKFREE_ALIGN_SIZE);
		while (lockfree_stack_pop(stack, node));
		_aligned_free(node);
	}

	int __cdecl lockfree_stack_empty(lockfree_stack *stack) {
		__asm {
			mov ebx, stack

			mov eax, [ebx]
			cmp eax, 0
			je return_true
			mov eax, 0
			jmp exit_function
			return_true :
			mov eax, 1
			exit_function :
		}
	}

	lockfree_queue * __cdecl lockfree_create_queue(lockfree_freelist *freelist) {
		__asm {
			push LOCKFREE_ALIGN_SIZE
			push TYPE lockfree_queue
			call _aligned_malloc
			add esp, 8

			cmp eax, 0

			jne alloc_malloc_ok

			jmp exit_function

			alloc_malloc_ok :
			mov ebx, 0
				mov[eax + 4], ebx
				mov[eax + 12], ebx
				mov ebx, freelist
				mov[eax + 16], ebx

				push eax

				push ebx
				call lockfree_freelist_alloc
				add esp, 4

				pop ebx
				mov[ebx], eax
				mov[ebx + 8], eax
				mov ecx, 0
				mov[eax], ecx
				mov[eax + 4], ecx
				mov eax, ebx

				exit_function :
		}
	}

	void __cdecl lockfree_destroy_queue(lockfree_queue *queue) {
		__asm {
			push queue
			call lockfree_queue_clear
			add esp, 4

			mov ebx, queue

			push[ebx]
			call _aligned_free
			add esp, 4

			push queue
			call _aligned_free
			add esp, 4
		}
	}

	void __cdecl lockfree_queue_push(lockfree_queue *queue, void *value) {
#define			register_queue				mm2
#define			register_value				mm3
#define			register_new_ptr			mm4
#define			register_tail				mm5
#define			register_next				mm6
#define			register_node				mm7
		__asm {

			mov ebx, queue

			mov edx, [ebx + 16]

			mov ecx, [edx + 8]

			push ecx

			push edx
			call lockfree_freelist_alloc
			add esp, 4


			movd register_queue, queue

			movd register_value, value

			movd register_node, eax

			pop ecx
			movd esi, register_value
			mov edi, eax
			add edi, 8

			rep movsb

			mov ecx, 0
			mov[eax], ecx


			loop_start :

			movd ebx, register_queue

				movq register_tail, [ebx + 8]


				movd ebx, register_tail

				movq register_next, [ebx]


				movq mm0, register_tail
				movd ebx, register_queue


				movq mm1, [ebx + 8]

				movd eax, mm0
				movd ebx, mm1

				cmp eax, ebx

				jne loop_start

				psrlq mm0, 32
				psrlq mm1, 32

				movd eax, mm0
				movd ebx, mm1

				cmp eax, ebx


				jne loop_start

				movd eax, register_next

				cmp eax, 0

				jne else_label

				movd edi, register_tail

				movq mm0, register_next
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				movd ebx, register_node
				movd ecx, edx
				inc ecx

				lock cmpxchg8b[edi]

				jnz loop_start

				movd edi, register_queue
				add edi, 8

				movq mm0, register_tail
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				mov ecx, edx
				inc ecx
				movd ebx, register_node

				lock cmpxchg8b[edi]

				jmp exit_function
				else_label :

			movd edi, register_queue
				add edi, 8

				movq mm0, register_tail
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				mov ecx, edx
				inc ecx
				movd ebx, register_next

				lock cmpxchg8b[edi]

				jmp loop_start
				exit_function :

			emms

		}
#undef			register_queue
#undef			register_value
#undef			register_new_ptr
#undef			register_tail
#undef			register_next
#undef			register_node
	}

	int __cdecl lockfree_queue_pop(lockfree_queue *queue, void *value) {
#define			register_queue					mm2
#define			register_value					mm3
#define			register_head					mm4
#define			register_tail					mm5
#define			register_next					mm6
#define			register_node_size				mm7
		__asm {

			mov ebx, queue
			movd register_queue, ebx

			mov ecx, [ebx + 16]

			mov edx, [ecx + 8]
			movd register_node_size, edx

			loop_start :

			movd ebx, register_queue

				movq register_head, [ebx]

				movq register_tail, [ebx + 8]

				movd ebx, register_head

				movq register_next, [ebx]

				movq mm0, register_head
				movd ebx, register_queue

				movq mm1, [ebx]

				movd eax, mm0
				movd ebx, mm1

				cmp eax, ebx

				jne loop_start

				psrlq mm0, 32
				psrlq mm1, 32

				movd eax, mm0
				movd ebx, mm1

				cmp eax, ebx

				jne loop_start

				movd eax, register_head
				movd ebx, register_tail

				cmp eax, ebx

				jne else_label

				movd eax, register_next

				cmp eax, 0

				jne fail_return_false

				mov eax, 0

				emms
				jmp exit_function

				fail_return_false :


			movd edi, register_queue
				add edi, 8

				movq mm0, register_tail
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				mov ecx, edx
				inc ecx
				movd ebx, register_next

				lock cmpxchg8b[edi]

				jmp loop_start

				else_label :


			movd esi, register_next

				cmp esi,0
				je loop_start
				add esi, 8

				mov edi, value
				movd ecx, register_node_size

				rep movsb
				movd edi, register_queue

				movq mm0, register_head
				movd eax, mm0
				psrlq mm0, 32
				movd edx, mm0
				mov ecx, edx
				inc ecx
				movd ebx, register_next

				lock cmpxchg8b[edi]
				jnz loop_start

				movd ebx, register_queue

				mov edx, [ebx + 16]
				movd ecx, register_head

				emms

				push ecx
				push edx
				call lockfree_freelist_free
				add esp, 8

				mov eax, 1

				exit_function:
		}
#undef			register_queue
#undef			register_value
#undef			register_head
#undef			register_tail
#undef			register_next
#undef			register_node_size
	}

	void __cdecl lockfree_queue_clear(lockfree_queue *queue) {
		unsigned int node_size = queue->freelist->node_size;
		unsigned char *node = (unsigned char *)_aligned_malloc(node_size, LOCKFREE_ALIGN_SIZE);
		while (lockfree_queue_pop(queue, node));
		_aligned_free(node);
	}

	int __cdecl lockfree_queue_empty(lockfree_queue *queue) {
		__asm {

			mov eax, queue

			mov ebx, [eax]

			mov ecx, [eax + 8]
			cmp ebx, ecx
			jne return_false

			mov ebx, [eax + 4]

			mov ecx, [eax + 12]
			cmp ebx, ecx
			jne return_false
			mov eax, 1
			jmp exit_function
			return_false :
			mov eax, 0
			exit_function :
		}
	}

	lockfree_ref_memory *__cdecl lockfree_create_ref_memory(lockfree_freelist *freelist,void *free_function) {
		__asm {
			push LOCKFREE_ALIGN_SIZE
			push TYPE lockfree_ref_memory
			call _aligned_malloc
			add esp, 8

			cmp eax,0
			je exit_function

			mov ebx,free_function
			mov [eax],ebx
			mov ebx,freelist
			mov [eax+4],ebx

			exit_function :
		}
	}

	void __cdecl lockfree_destroy_ref_memory(lockfree_ref_memory *ref_memory) {
		__asm {
			push ref_memory
			call _aligned_free
			add esp,4
		}
	}

	lockfree_ref_memory_pointer __cdecl lockfree_ref_memory_alloc(lockfree_ref_memory *ref_memory,void *node) {
		__asm {
			mov ebx,ref_memory

			push ebx
			push [ebx+4]

			call lockfree_freelist_alloc
			add esp,4

			pop ebx

			cmp eax,0
			je exit_function

			mov [eax],ebx
			mov ecx,1
			mov [eax+4],ecx
			mov ecx,node
			mov [eax+8],ecx

			exit_function:
		}
	}

	void *__cdecl lockfree_ref_memory_get(lockfree_ref_memory_pointer ref_memory_pointer) {
		__asm {
			mov ebx,[ref_memory_pointer]
			mov eax,[ebx+8]
		}
	}

	void __cdecl lockfree_ref_memory_add_ref(lockfree_ref_memory_pointer ref_memory_pointer) {
		__asm {
			mov ebx,ref_memory_pointer
			add ebx,4
			mov eax,[ebx]
			cas_loop_start:
			mov ecx,eax
			inc ecx
			lock cmpxchg [ebx],ecx
			jnz cas_loop_start
		}
	}

	void __cdecl lockfree_ref_memory_sub_ref(lockfree_ref_memory_pointer ref_memory_pointer) {
		__asm {
			mov ebx, ref_memory_pointer
			add ebx, 4
			mov eax, [ebx]
			cas_loop_start:
			mov ecx, eax
			dec ecx
			lock cmpxchg[ebx], ecx
			jnz cas_loop_start
			cmp ecx,0
			jne exit_function
			sub ebx,4
			mov edx,[ebx]
			mov ecx,[edx]

			cmp ecx,0
			je push_to_freelist

			push ebx
			push edx
			push [ebx+8]
			call ecx
			add esp,4
			
			pop edx
			pop ebx

			push_to_freelist:
			push ebx
			push [edx+4]

			call lockfree_freelist_push
			add esp,8

			exit_function:
		}
	}
}

