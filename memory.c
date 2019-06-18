/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include "compiler.h"

/*
	ALLOC_VAR_NUM:    # of variables used for allocation of memory (string/dimension).
	                  0 is for A, 1 is for B, ... , and 25 is for Z.
	                  This number also includes temporary area used for string construction etc.
	                  Temporary area is cleared every line of BASIC code in alloc_memory().
	ALLOC_BLOCK_NUM:  # of blocks that can be used for memory allocation.
	                  This # includes the ones for ALLOC_VAR_NUM, ALLOC_PCG_BLOCK etc, ALLOC_LNV_BLOCK,
	                  ALLOC_PERM_BLOCK.
	                  After ALLOC_VAR_NUM area, dedicated memory area and permanent area follows.
	ALLOC_PERM_BLOCK: Start # of permanent blocks.
                      The blocks after this number is permanently stored.
                      Therefore, it must be released when it's not used any more.
*/

static int g_temp_var_num_candidate=ALLOC_PERM_BLOCK;

#define DELETE_LIST_SIZE 10
static int g_deleted_num;
static int g_deleted_pointer[DELETE_LIST_SIZE];
static int g_deleted_size[DELETE_LIST_SIZE];

void register_deleted_block(int pointer, int size){
	// There is maximum
	if (DELETE_LIST_SIZE<=g_deleted_num) return;
	if (g_deleted_num) {
		// Avoid duplication
		if (g_deleted_pointer[g_deleted_num-1]==pointer) return;
	}
	g_deleted_pointer[g_deleted_num]=pointer;
	g_deleted_size[g_deleted_num]=size;
	g_deleted_num++;
} 

void set_free_area(void* begin, void* end){
	// Initialize heap area
	int i;
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_size[i]=0;
	}
	g_heap_mem=(int*)begin;
	g_max_mem=(int)((end-begin)/4);
	g_deleted_num=0;
}

void* calloc_memory(int size, int var_num){
	int i;
	void* ret;
	// Allocate memory
	ret=alloc_memory(size,var_num);
	// Fill zero in allocated memory
	for(i=0;i<size;i++){
		((int*)ret)[i]=0;
	}
	// return pointer to allocated memory
	return ret;
}
void* alloc_memory(int size, int var_num){
	// Remove temporary blocks once a line.
	asm volatile("nop");
	asm volatile("bltz $s6,_alloc_memory_main"); // Skip if $s6<0
	// Following code will be activated after setting $s6 register
	// every line and after label statement.
	asm volatile("lui $v0,0x8000");
	asm volatile("or $s6,$v0,$s6");              // $s6=0x80000000|$s6;
	// Remove all temporary blocks
	// Note that g_var_size is short integer.
	// Note that ALLOC_VAR_NUM is used here (but not ALLOC_BLOC_NUM)
	//	for(i=26;i<ALLOC_VAR_NUM;i++)g_var_size[i]=0;
	asm volatile("addiu $v0,$zero,%0"::"n"((ALLOC_VAR_NUM-26)/2)); // $v0=(ALLOC_VAR_NUM-26)/2
	asm volatile("la $v1,%0"::"i"(&g_var_size[0]));                // $v1=g_var_size
	asm volatile("loop:");
	asm volatile("sw $zero,(26*2)($v1)");                          // $v1[26]=0, $v1[27]=0
	asm volatile("addiu $v0,$v0,-1");                              // $v0--
	asm volatile("addiu $v1,$v1,4");                               // $v1+=2
	asm volatile("bne $v0,$zero,loop");                            // loop if 0<$v0
	asm volatile("b _alloc_memory_main");
}
void* _alloc_memory_main(int size, int var_num){
	int i,j,candidate;
	// Assign temp var number
	if (var_num<0) {
		// Use ALLOC_VAR_NUM here but not ALLOC_BLOCK_NUM
		for(i=26;i<ALLOC_VAR_NUM;i++){
			if (g_var_size[i]==0) {
				var_num=i;
				break;
			}
		}
		if (var_num<0) {
			err_str_complex();
			return 0;
		}
	}
	// Clear var to be assigned.
	g_var_size[var_num]=0;
	g_var_pointer[var_num]=0;
	while(1){
		// Try the block previously deleted, not for temporary block.
		// This is for fast allocation of memory for class object.
		if (var_num<26 || ALLOC_VAR_NUM<=var_num) {
			candidate=0;
			while(g_deleted_num){
				// Check if the last deleted block fits
				// If not, these cannot be used anymore
				g_deleted_num--;
				if (size<=g_deleted_size[g_deleted_num]) {
					candidate=g_deleted_pointer[g_deleted_num];
					break;
				}
			}
			if (candidate || g_deleted_num) {
				// Candidate found
				break;
			}
		}
		// Try the block after last block
		candidate=0;
		for(i=0;i<ALLOC_BLOCK_NUM;i++){
			if (g_var_size[i]==0) continue;
			if (candidate<=g_var_pointer[i]) {
				candidate=g_var_pointer[i]+g_var_size[i];
			}
		}
		if (candidate+size<=g_max_mem) {
			// Check after deleted block
			j=candidate;
			for(i=0;i<g_deleted_num;i++){
				if (j<g_deleted_pointer[i]+g_deleted_size[i]) {
					j=g_deleted_pointer[i]+g_deleted_size[i];
				}
			}
			if (j+size<=g_max_mem) {
				// Candidate block found after previously deleted blokcs
				candidate=j;
				break;
			} else {
				// Candidate is before previously deleted blocks,
				// and there is no candidate block after previously deleted blocks.
				// Therefore, use the current candidate, which may invade previously
				// deleted blocks. Therefore, erase the previously deleted blocks list.
				g_deleted_num=0;
				break;
			}
		}
		// Peviously deleted blocks cannot be used any more
		g_deleted_num=0;
		// Check between blocks
		// Note that there is at least one block with zero pointer and zero size (see above).
		for(i=0;i<ALLOC_BLOCK_NUM;i++){
			// Candidate is after this block.
			candidate=g_var_pointer[i]+g_var_size[i];
			// Check if there is an overlap.
			for(j=0;j<ALLOC_BLOCK_NUM;j++){
				if (g_var_size[j]==0) continue;
				if (candidate+size<=g_var_pointer[j]) continue;
				if (g_var_pointer[j]+g_var_size[j]<=candidate) continue;
				// This block overlaps with the candidate
				candidate=-1;
				break;
			}
			if (0<=candidate && candidate+size<=g_max_mem) {
				// Available block found
				break;
			} else {
				candidate=-1;
			}
		}
		if (0<=candidate) break;
		// New memory block cannot be allocated.
		err_no_mem();
		return 0;
	}
	// Available block found.
	g_var_pointer[var_num]=candidate;
	g_var_size[var_num]=size;
	g_var_mem[var_num]=(int)(&(g_heap_mem[candidate]));
	return (void*)g_var_mem[var_num];
}

void free_temp_str(char* str){
	int i,pointer;
	if (!str) return;
	pointer=(int)str-(int)g_heap_mem;
	pointer>>=2;
	for(i=26;i<ALLOC_VAR_NUM;i++){
		if (g_var_pointer[i]==pointer) {
			if (g_var_size[i] && g_var_mem[i]==(int)str) {
				register_deleted_block(pointer,g_var_size[i]);
				g_var_size[i]=0;
			}
		}
	}
}

void free_non_temp_str(char* str){
	int i,pointer;
	if (!str) return;
	pointer=(int)str-(int)g_heap_mem;
	pointer>>=2;
	for(i=0;i<26;i++){
		if (g_var_pointer[i]==pointer) {
			if (g_var_size[i] && g_var_mem[i]==(int)str) {
				register_deleted_block(pointer,g_var_size[i]);
				g_var_size[i]=0;
				g_var_mem[i]=0;
			}
		}
	}
	for(i=ALLOC_VAR_NUM;i<ALLOC_BLOCK_NUM;i++){
		if (g_var_pointer[i]==pointer) {
			if (g_var_size[i] && g_var_mem[i]==(int)str) {
				register_deleted_block(pointer,g_var_size[i]);
				g_var_size[i]=0;
				g_var_mem[i]=0;
				if (ALLOC_PERM_BLOCK<=i) g_temp_var_num_candidate=i;
			}
		}
	}
}

void free_perm_str(char* str){
	int i,pointer;
	if (!str) return;
	pointer=(int)str-(int)g_heap_mem;
	pointer>>=2;
	// Search permanent block and delete a block if found.
	for(i=ALLOC_PERM_BLOCK;i<ALLOC_BLOCK_NUM;i++){
		if (g_var_pointer[i]==pointer) {
			if (g_var_size[i] && g_var_mem[i]==(int)str) {
				register_deleted_block(pointer,g_var_size[i]);
				g_var_size[i]=0;
				g_temp_var_num_candidate=i;
				break;
			}
		}
	}
}

void move_to_perm_block(int var_num){
	int i;
	// Find available permanent block
	i=get_permanent_var_num();
	// Available block found.
	// Copy value from variable.
	g_var_size[i]=g_var_size[var_num];
	g_var_pointer[i]=g_var_pointer[var_num];
	g_var_mem[i]=g_var_mem[var_num];
	// Clear variable
	g_var_size[var_num]=0;
	g_var_mem[var_num]=0;
}

int move_from_perm_block_if_exists(int var_num){
	int i,pointer;
	pointer=(int)g_var_mem[var_num]-(int)g_heap_mem;
	pointer>>=2;
	// Find stored block
	for (i=ALLOC_PERM_BLOCK;i<ALLOC_BLOCK_NUM;i++){
		if (0<g_var_size[i] && g_var_pointer[i]==pointer) break;
	}
	if (ALLOC_BLOCK_NUM<=i) return 0; // Not found
	// Stored block found.
	// Replace pointer
	g_var_size[var_num]=g_var_size[i];
	g_var_pointer[var_num]=g_var_pointer[i];
	// Clear block
	g_var_size[i]=0;
	g_temp_var_num_candidate=i;
	return 1;
}

void move_from_perm_block(int var_num){
	if (move_from_perm_block_if_exists(var_num)) return; // Found
	err_unknown(); // Not found
}

int get_permanent_var_num(){
	int i;
	// Try candidate first
	if (!g_var_size[g_temp_var_num_candidate]) {
		if (ALLOC_PERM_BLOCK<g_temp_var_num_candidate && 
				g_temp_var_num_candidate<ALLOC_BLOCK_NUM) {
			return g_temp_var_num_candidate++;
		}
	}
	// Candidate is not available. Seek all.
	for (i=ALLOC_PERM_BLOCK;i<ALLOC_BLOCK_NUM;i++) {
		if (g_var_size[i]==0) {
			g_temp_var_num_candidate=i+1;
			return i;
		}
	}
	err_no_block();
	return 0;
}

int get_varnum_from_address(void* address){
	int i;
	for (i=0;i<ALLOC_BLOCK_NUM;i++){
		if (g_var_mem[i]==(int)address) return i;
	}
	// not found
	return -1;
}

void* lib_calloc_memory(int size){
	// Allocate memory and return address
	return calloc_memory(size,get_permanent_var_num());
}

void lib_delete(int* object){
	// Remove region that fit to object
	free_non_temp_str((char*)object);
}
