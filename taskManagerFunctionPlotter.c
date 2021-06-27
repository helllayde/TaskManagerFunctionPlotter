/*
 * Copyright (C) 2018 helllayde
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 *	Definition of the function pointer type
 */
typedef float(*function_t)(float);

/*
 *	Structure of a memory block
 *
 *	sz = size in bytes of the memory block
 *	mem = pointer to the allocated memory
 *	next = next block in the list
 */
typedef struct s_block {
	size_t sz;
	void* mem;
	struct s_block* next;
}mblock_t;

/*
 *	Get the available memory size
 *
 *	This is the only windows-specific function in this code snippet
 *	Returns the available memory size in bytes
 */
DWORDLONG getMemorySize() {
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return (status.ullTotalPhys * (1 - (float)status.dwMemoryLoad / 100u));
}

/*
 *	Frees all the linked memory blocks
 *
 *	memory = head of the list
 */
void destroyMemory(mblock_t* memory) {
	mblock_t* block;

	for (block = memory, memory = memory->next; memory; block = memory, memory = memory->next) {
		if (block->mem)
			free(block->mem);
		free(block);
	}
}

/*
 *	Memory manager
 *
 *	memory = head of the block list
 *	totalSpace = total allocated memory
 *	requested = total requested memory
 */
void reallocMemory(mblock_t* memory, size_t totalSpace, size_t requested) {
	unsigned i;
	mblock_t* block, * prev;
	size_t delta;	//Difference between total space and requested space

	if (requested > totalSpace) {	//Allocation Mechanism
		delta = requested - totalSpace;

		if (delta > 1024 * 1024) {	//Allocates in spans of 1MB each
			float iterations = (float)delta / (1024 * 1024);
			unsigned spans = (unsigned)iterations;

			if (iterations != spans)
				spans++;

			for (i = 0; i < spans; i++)
				reallocMemory(memory, totalSpace, totalSpace + delta / spans);

		}
		else {

			if (!(memory->mem)) {
				memory->mem = malloc(delta);
				memory->sz = delta;
			}

			block = memory->next;
			memory->next = (mblock_t*)malloc(sizeof(mblock_t));
			memory->next->mem = malloc(delta);
			memory->next->sz = delta;
			memory->next->next = block;
		}
	}
	else if (requested < totalSpace) {	//Deallocation mechanism
		delta = totalSpace - requested;

		for (block = memory, prev = NULL; block; ) {
			if (delta == 0)
				return;

			if (block->mem) {
				if (block->sz <= delta) {	//free more little blocks
					delta -= block->sz;
					free(block->mem);
					if (prev) {				//is this the frist block?
						prev->next = block->next;
						free(block);
						block = prev->next;
					}
					else {
						block->mem = NULL;
						block->sz = 0;
						prev = block;
						block = block->next;
					}
				}
				else {						//free one huge block
					free(block->mem);
					block->mem = malloc(block->sz - delta);
					block->sz = block->sz - delta;
					return;
				}
			}
			else {
				prev = block;
				block = block->next;
			}
		}
	}
	else
		return;
}

/*
 *	Plot a given function in the Task manager
 *
 *	maxMemory = Max occupable memory
 *	minY = minimum value of Y
 *	maxY = maximum value of Y
 *	minX = left side of the interval
 *	maxX = right side of the interval
 *	step = step between two values
 *	timeStep = time to await between two steps (adjust the graph shape)
 *	f = function pointer to the function to plot
 */
void printGraph(size_t maxMemory, float minY, float maxY, float minX, float maxX, float step, unsigned timeStep, function_t f) {
	float x, y;
	mblock_t* memory = (mblock_t*)malloc(sizeof(mblock_t));
	size_t totMem = 0, mem;

	memory->mem = NULL;
	memory->next = NULL;
	memory->sz = 0;

	for (x = minX; x <= maxX; x += step) {
		y = f(x);
		mem = (size_t)(((y - minY) / maxY) * (float)maxMemory);
		printf("Plotting f(%.2f) = %.2f\tMemory usage: %.2fGB...\n", x, y, (float)mem / (1024 * 1024 * 1024));
		reallocMemory(memory, totMem, mem);

		Sleep(timeStep);

		totMem = mem;
	}

	destroyMemory(memory);
	free(memory);
}

/*
 *	Penis shaped function
 */
float dickFunction(float x) {
	return (float)fabs(sinf(x)) + 5 * expf(-powf(x, 100)) * cosf(x);
}

/*
 *	Parabola function
 */
float parabola(float x) {
	return x * x;
}

/*
 *	Entry pount
 */
int main(int argc, char* argv[]) {
	DWORDLONG availableMemory = getMemorySize();
	printf("Available memory: %.2fGB\n", (float)availableMemory / (1024 * 1024 * 1024));
	printf("Starting plotting...\n");
	printGraph((size_t)(availableMemory * 0.9f), 0.0f, 5.0f, -3.0f, 3.0f, 0.001f, 3, dickFunction);
	//printGraph((size_t)(availableMemory * 0.9f), 0.0f, 9.0f, -3.0f, 3.0f, 0.001f, 3, parabola);
	printf("Done.\n");
	getchar();
	return 0;
}