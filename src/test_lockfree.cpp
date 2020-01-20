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
#include <iostream>
#include <thread>
#include <lockfree.h>
#include <set>
#include <atomic>
#include <queue>
#include <mutex>

#include <boost/lockfree/stack.hpp>
#include <boost/lockfree/queue.hpp>
#include <Windows.h>

#define READ_THREAD_COUNT							1
#define WRITE_THREAD_COUNT							20

#define QUEUE_ITEM_COUNT_PER_THREAD					2000000

#define QUEUE_ITEM_COUNT							(WRITE_THREAD_COUNT*QUEUE_ITEM_COUNT_PER_THREAD)

std::queue<unsigned int> std_queue;
std::mutex std_queue_mutex;

//boost::lockfree::queue<unsigned int> boost_queue(QUEUE_ITEM_COUNT+10);

lockfree_freelist *freelist;
lockfree_queue *queue;
std::atomic<unsigned int> read_cnt;
std::atomic<unsigned int> write_cnt;

std::thread *read_threads[128];
std::thread *write_threads[128];

void TestForStdQueue() {
	auto start_time = std::chrono::high_resolution_clock::now();

	read_cnt = 0;

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		read_threads[i] = new std::thread([]() {
			unsigned int val;
			unsigned int last_cnt;
			while (true) {
				std_queue_mutex.lock();
				if (std_queue.empty()==false) {
					val = std_queue.front();
					std_queue.pop();
					std_queue_mutex.unlock();
					do {
						last_cnt = read_cnt;
					} while (!read_cnt.compare_exchange_strong(last_cnt, last_cnt + 1));
					if (last_cnt + 1 == QUEUE_ITEM_COUNT)
						break;
				}
				else if (read_cnt == QUEUE_ITEM_COUNT) {
					std_queue_mutex.unlock();
					break;
				}
				else
					std_queue_mutex.unlock();
			}
		});
	}

	write_cnt = 0;

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		write_threads[i] = new std::thread([]() {
			unsigned int val;
			for (auto i = 0; i < QUEUE_ITEM_COUNT_PER_THREAD;i++) {
				val = i;
				std_queue_mutex.lock();
				std_queue.push(val);
				std_queue_mutex.unlock();
			}
		});
	}

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		read_threads[i]->join();
	}

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		write_threads[i]->join();
	}

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time);

	std::cout << "=====================================================================" << std::endl;
	std::cout << READ_THREAD_COUNT << " read threads " << WRITE_THREAD_COUNT << " write threads " << QUEUE_ITEM_COUNT << " items" << std::endl;
	std::cout << "std::queue+std::mutex time:";
	std::cout << duration.count() << "ms" << std::endl;

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		delete read_threads[i];
	}

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		delete write_threads[i];
	}
}

void TestForBoostLockFreeQueue() {
/*	auto start_time = std::chrono::high_resolution_clock::now();
	read_cnt = 0;

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		read_threads[i] = new std::thread([]() {
			unsigned int val;
			unsigned int last_cnt;
			while (true) {
				if (boost_queue.pop(val)) {
					do {
						last_cnt = read_cnt;
					} while (!read_cnt.compare_exchange_strong(last_cnt, last_cnt + 1));
					if (last_cnt + 1 == QUEUE_ITEM_COUNT)
						break;
				}
				else if (read_cnt == QUEUE_ITEM_COUNT)
					break;
				Sleep(0);
			}
		});
	}

	write_cnt = 0;

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		write_threads[i] = new std::thread([]() {
			unsigned int val;
			for(auto i=0;i< QUEUE_ITEM_COUNT_PER_THREAD;i++){
				val = i;
				boost_queue.push(val);
				Sleep(0);
			}
		});
	}

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		read_threads[i]->join();
	}

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		write_threads[i]->join();
	}

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time);

	std::cout << "=====================================================================" << std::endl;
	std::cout << READ_THREAD_COUNT << " read threads " << WRITE_THREAD_COUNT << " write threads " << QUEUE_ITEM_COUNT << " items" << std::endl;
	std::cout << "boost::lockfree::queue time:";
	std::cout << duration.count() << "ms" << std::endl;

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		delete read_threads[i];
	}

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		delete write_threads[i];
	}*/
}

void TestForAsmLockFreeQueue() {
	freelist = lockfree_create_freelist(4);

	queue = lockfree_create_queue(freelist);

	for (auto i = 0; i < QUEUE_ITEM_COUNT+10; i++) {
		unsigned int val;
		lockfree_queue_push(queue, &val);
	}

	for (auto i = 0; i < QUEUE_ITEM_COUNT+10; i++) {
		unsigned int val;
		lockfree_queue_pop(queue, &val);
	}

	auto start_time = std::chrono::high_resolution_clock::now();

	read_cnt = 0;

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		read_threads[i] = new std::thread([]() {
			unsigned int val;
			unsigned int last_cnt;
			auto last_time=std::chrono::high_resolution_clock::now();
			while (true) {
				if (lockfree_queue_pop(queue, &val)) {
					do {
						last_cnt = read_cnt;
					} while (!read_cnt.compare_exchange_strong(last_cnt, last_cnt + 1));
					if (last_cnt + 1 == QUEUE_ITEM_COUNT)
						break;
				}
				else if (read_cnt == QUEUE_ITEM_COUNT)
					break;
			}
		});
	}

	write_cnt = 0;

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		write_threads[i] = new std::thread([]() {
			unsigned int val;
			
			for(auto i=0;i< QUEUE_ITEM_COUNT_PER_THREAD;i++){
				val = i;
				lockfree_queue_push(queue, &val);
				//MemoryBarrier();
			}
		});
	}

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		read_threads[i]->join();
	}

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		write_threads[i]->join();
	}

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time);

	std::cout << "=====================================================================" << std::endl;
	std::cout << READ_THREAD_COUNT << " read threads " << WRITE_THREAD_COUNT << " write threads " << QUEUE_ITEM_COUNT << " items" << std::endl;
	std::cout << "asm lockfree queue time:";
	std::cout << duration.count() << "ms" << std::endl;

	for (auto i = 0; i < READ_THREAD_COUNT; i++) {
		delete read_threads[i];
	}

	for (auto i = 0; i < WRITE_THREAD_COUNT; i++) {
		delete write_threads[i];
	}

	
	lockfree_destroy_queue(queue);
	unsigned int cnt = 0;
	lockfree_freelist_node *ptr= freelist->top.ptr;

	while (ptr) {
		cnt++;
		ptr = ptr->next.ptr;
	}
	std::cout << cnt << std::endl;
	lockfree_destroy_freelist(freelist);
}

extern "C" {
	void __cdecl Free(void *node) {
		std::cout << (char *)node << std::endl;
		delete[]node;
	}
}



int main() {
	/*std::cout << sizeof(unsigned short) << std::endl;
	TestForStdQueue();
	TestForBoostLockFreeQueue();
	TestForAsmLockFreeQueue();*/
	freelist = lockfree_create_freelist(sizeof(lockfree_ref_memory_node));
	//lockfree_vector *vector = lockfree_create_vector(freelist);
	//lockfree_stack *stack = lockfree_create_stack(freelist);
	lockfree_ref_memory *ref_memory = lockfree_create_ref_memory(freelist, Free);

	char *p=new char[100];
	strcpy(p, "hello world!");
	auto pointer = lockfree_ref_memory_alloc(ref_memory, p);
	lockfree_ref_memory_add_ref(pointer);
	lockfree_ref_memory_sub_ref(pointer);
	lockfree_ref_memory_sub_ref(pointer);


	lockfree_destroy_ref_memory(ref_memory);
	lockfree_destroy_freelist(freelist);
	return 0;
}