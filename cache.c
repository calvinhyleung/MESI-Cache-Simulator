#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "cache.h"
#include "main.h"

/* cache configuration parameters */
static int cache_usize = DEFAULT_CACHE_SIZE;                        // input 
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;             // input 
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;  // calculated
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;
static int num_core = DEFAULT_NUM_CORE;                             // input 

/* cache model data structures */
/* max of 8 cores */
static cache mesi_cache[8];
static cache_stat mesi_cache_stat[8];

void print_cache_info(int i){ 
  printf("cache %d info:\n", i);
  printf("\tid: %d\n", mesi_cache[i].id);
  printf("\tsize: %d\n", mesi_cache[i].size);
  //printf("\thead: %u\n", mesi_cache[i].LRU_head);
  printf("\thead tag: %.8x\n", mesi_cache[i].LRU_head->tag);
  //printf("\ttail: %u\n", mesi_cache[i].LRU_tail);
  printf("\ttail tag: %.8x\n", mesi_cache[i].LRU_tail->tag);
  printf("\tcontent: %d\n", mesi_cache[i].cache_contents);
}

void print_cache_list(){
  for (int i = 0; i < num_core; i ++){
  printf("cache %d linked list:\n", i);
  int j = 0;
  Pcache_line node = mesi_cache[i].LRU_head;
  while(node){
    printf("\tnode %d \ttag: %.8x, state: %d\n", j, node->tag, node->state);
    node = node->LRU_next;
    j++;
  }
  }
}

void print_basic(int i, int addr) {
  printf("\naccess store:\n");
  printf("\taddr: %.8x\n", addr);
  printf("\ti: %d\n", i);
}

/************************************************************/
void set_cache_param(param, value)
  int param;
  int value;
{
  switch (param) {
  case NUM_CORE:
    num_core = value;
    break;
  case CACHE_PARAM_BLOCK_SIZE:
    cache_block_size = value;
    words_per_block = value / WORD_SIZE;
    break;
  case CACHE_PARAM_USIZE:
    cache_usize = value;
    break;
  default:
    printf("error set_cache_param: bad parameter value\n");
    exit(-1);
  }
}
/************************************************************/

/************************************************************/
void init_cache()
{
  int i;

  /* initialize the cache, and cache statistics data structures */
  for(i = 0; i < num_core; i++)
  {
    printf("initiating cache %d\n", i);
    mesi_cache[i].id = i; 
    mesi_cache[i].size = 0;
    // mesi_cache[i].LRU_head = (Pcache_line )malloc(sizeof(cache_line)); 
    // mesi_cache[i].LRU_head->LRU_next = (Pcache_line)NULL;
    // mesi_cache[i].LRU_head->LRU_prev = (Pcache_line)NULL;
    // mesi_cache[i].LRU_tail = (Pcache_line )malloc(sizeof(cache_line));
    // mesi_cache[i].LRU_tail->LRU_next = (Pcache_line)NULL;
    // mesi_cache[i].LRU_tail->LRU_prev = (Pcache_line)NULL;
    mesi_cache[i].LRU_head = (Pcache_line)NULL; 
    mesi_cache[i].LRU_tail = (Pcache_line)NULL;
    mesi_cache[i].cache_contents = 0; 
    // print_cache_info(i);
    print_cache_list(i);
    mesi_cache_stat[i].accesses = 0;
    mesi_cache_stat[i].misses = 0;
    mesi_cache_stat[i].replacements = 0;
    mesi_cache_stat[i].demand_fetches = 0;
    mesi_cache_stat[i].copies_back = 0;
    mesi_cache_stat[i].broadcasts = 0;
  }
}
/************************************************************/

/************************************************************/
void perform_access(addr, access_type, pid)
     unsigned addr, access_type, pid;
  
{
  int i, idx;
  Pcache c;

  /* handle accesses to the mesi caches */
  switch(access_type)
  {
    case TRACE_LOAD:
      perform_access_load(addr, pid);
      break;
    case TRACE_STORE:
      perform_access_store(addr, pid);
      break;
  }

}
/************************************************************/

/************************************************************/
void flush()
{
  for (int i = 0; i < num_core; i ++) {
    Pcache_line node = mesi_cache[i].LRU_head;
    while(node){
      if (node->state == STATE_MODIFIED){
        mesi_cache_stat[i].copies_back += 1; 
      }
      node = node->LRU_next;
    }
  }
}
/************************************************************/

/************************************************************/
void delete(head, tail, item)
  Pcache_line *head, *tail;
  Pcache_line item;
{
  if (item->LRU_prev) {
    item->LRU_prev->LRU_next = item->LRU_next;
  } else {
    /* item at head */
    *head = item->LRU_next;
  }

  if (item->LRU_next) {
    item->LRU_next->LRU_prev = item->LRU_prev;
  } else {
    /* item at tail */
    *tail = item->LRU_prev;
  }
}
/************************************************************/

/************************************************************/
/* inserts at the head of the list */
void insert(head, tail, item)
  Pcache_line *head, *tail;
  Pcache_line item;
{
  item->LRU_next = *head;
  item->LRU_prev = (Pcache_line)NULL;
  if (item->LRU_next)
    item->LRU_next->LRU_prev = item;
  else
    *tail = item;
  *head = item;
}
/************************************************************/

/************************************************************/
void dump_settings()
{
  printf("Cache Settings:\n");
  printf("\tSize: \t%d\n", cache_usize);
  printf("\tBlock size: \t%d\n", cache_block_size);
}
/************************************************************/

/************************************************************/
void print_stats()
{
  int i;
  int demand_fetches = 0;
  int copies_back = 0;
  int broadcasts = 0;

  printf("*** CACHE STATISTICS ***\n");

  for (i = 0; i < num_core; i++) {
    printf("  CORE %d\n", i);
    printf("  accesses:  %d\n", mesi_cache_stat[i].accesses);
    printf("  misses:    %d\n", mesi_cache_stat[i].misses);
    printf("  miss rate: %f (%f)\n", 
	   (float)mesi_cache_stat[i].misses / (float)mesi_cache_stat[i].accesses,
	   1.0 - (float)mesi_cache_stat[i].misses / (float)mesi_cache_stat[i].accesses);
    printf("  replace:   %d\n", mesi_cache_stat[i].replacements);
  }

  printf("\n");
  printf("  TRAFFIC\n");
  for (i = 0; i < num_core; i++) {
    demand_fetches += mesi_cache_stat[i].demand_fetches;
    copies_back += mesi_cache_stat[i].copies_back;
    broadcasts += mesi_cache_stat[i].broadcasts;
  }
  printf("  demand fetch (words): %d\n", (demand_fetches)*cache_block_size/WORD_SIZE);
  /* number of broadcasts */
  printf("  broadcasts:           %d\n", broadcasts);
  printf("  copies back (words):  %d\n", copies_back*words_per_block);
}
/************************************************************/

/************************************************************/
void init_stat(Pcache_stat stat)
{
  stat->accesses = 0;
  stat->misses = 0;
  stat->replacements = 0;
  stat->demand_fetches = 0;
  stat->copies_back = 0;
  stat->broadcasts = 0;
}
/************************************************************/

/************************************************************/
void remove_tail(int i) {
  if (mesi_cache[i].LRU_head == mesi_cache[i].LRU_tail) { // if only one node 
    mesi_cache[i].LRU_head = (Pcache_line)NULL; 
    mesi_cache[i].LRU_tail = (Pcache_line)NULL;
  } else {
    Pcache_line prev = mesi_cache[i].LRU_tail->LRU_prev;
    prev->LRU_next = (Pcache_line)NULL;; 
    mesi_cache[i].LRU_tail = prev;
  }
}

Pcache_line get_node(int i, int addr){
  Pcache_line node = mesi_cache[i].LRU_head;
  while(node){
    if (node->tag == addr){
      return node;
    }
    node = node->LRU_next;
  }
  return NULL;
}

int is_address_in_list(int i, int addr){
  Pcache_line node = mesi_cache[i].LRU_head;
  while(node){
    if (node->tag == addr){
      return TRUE;
    }
    node = node->LRU_next;
  }
  return FALSE;
}

void bus_read_miss(int i, int addr) {
  Pcache_line node = mesi_cache[i].LRU_head;
  while (node) {
    if ((node->state == STATE_EXCLUSIVE) && (node->tag == addr)){
      // EXCLUSIVE to SHARED
      node->state = STATE_SHARED;
    }
    if ((node->state == STATE_MODIFIED) && (node->tag == addr)){
      /***** WRITE BACK *****/
      // increment writeback counter 
      mesi_cache_stat[i].copies_back += 1;
      // MODIFIED to SHARED
      node->state = STATE_SHARED;
    }
    node = node->LRU_next;
  }
}

void bus_write_miss(int i, int addr) {
  Pcache_line node = mesi_cache[i].LRU_head;
  while (node) {
    if ((node->state == STATE_MODIFIED) && (node->tag == addr)){
      /***** WRITE BACK *****/
      // increment writeback counter 
      mesi_cache_stat[i].copies_back += 1;
      // MODIFIED to SHARED
      node->state = STATE_INVALID;
    } else if (node->tag == addr) {
      // EXCLUSIVE, SHARED, INVALID to INVALID
      node->state = STATE_INVALID;
    }
    node = node->LRU_next;
  }
}

void bus_write_hit(int i, int addr) {
  Pcache_line node = mesi_cache[i].LRU_head;
  while (node) {
    if ((node->state == STATE_SHARED) && (node->tag == addr)){
      // SHARED to INVALID
      node->state = STATE_INVALID;
    }
    node = node->LRU_next;
  }
}

void bus_transaction(int transaction_type, int sender, int addr){
  switch (transaction_type){
    case BUS_READ_MISS:
      for (int i = 0; i < num_core; i ++) {
        if (i != sender) bus_read_miss(i, addr); 
      }
    break;
    case BUS_WRITE_MISS:
      for (int i = 0; i < num_core; i ++) {
        if (i != sender) bus_write_miss(i, addr); 
      }
    break;
    case BUS_WRITE_HIT:
      for (int i = 0; i < num_core; i ++) {
        if (i != sender) bus_write_hit(i, addr); 
      }
    break;
  }
}
/* the function finds where which core the exclusive block is, return -1 if none*/
int no_exclusive(int sender, int addr) {
  for(int i = 0; i < num_core; i++){
    if (i != sender) {
      Pcache_line node = mesi_cache[i].LRU_head;
      while(node){
        if (node->tag == addr && node->state == STATE_EXCLUSIVE){
          return FALSE;
        }
        node = node->LRU_next;
      }
    }
  }
  return TRUE; 
}

void perform_access_store(int addr, int i)
{
  mesi_cache_stat[i].accesses += 1;
  //print_basic(i, addr);

  int num_shift = LOG2(cache_block_size);
  addr = (addr >> num_shift);

  Pcache_line new_cache_line = (Pcache_line )malloc(sizeof(cache_line));
  new_cache_line->tag = addr;
  new_cache_line->state = STATE_MODIFIED;

  Pcache_line node = get_node(i, addr);
  if (node == NULL) {
    /******************** WRITE MISS (NOT in cache) ********************/
    // increment misses counter
    mesi_cache_stat[i].misses += 1;
    // increment fetch 
    mesi_cache_stat[i].demand_fetches += 1;
    /***** if cache is full, remove the tail, writeback if tail is modified *****/
    if (mesi_cache[i].cache_contents >= cache_usize / cache_block_size){ 
      // increment replacement counter
      mesi_cache_stat[i].replacements += 1;
      // get tail info
      unsigned tail_tag = mesi_cache[i].LRU_tail->tag;
      int tail_state = mesi_cache[i].LRU_tail->state;
      // write back if modified 
      if (tail_state == STATE_MODIFIED){
        /***** WRITE BACK *****/
        // increment writeback counter 
        mesi_cache_stat[i].copies_back += 1; 
      }
      // remove tail 
      remove_tail(i);
      mesi_cache[i].cache_contents -= 1;
    } 
    // insert new block 
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
    // write allocate
    mesi_cache[i].cache_contents += 1;
    
    bus_transaction(BUS_WRITE_MISS, i, addr);
    mesi_cache_stat[i].broadcasts += 1;

  } else if (node->state == STATE_INVALID) {
    /******************** WRITE MISS (INVALID) ********************/
    // increment misses counter
    mesi_cache_stat[i].misses += 1;
    // increment fetch 
    mesi_cache_stat[i].demand_fetches += 1;

    delete(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, node);
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
    bus_transaction(BUS_WRITE_MISS, i, addr);
    mesi_cache_stat[i].broadcasts += 1;
  } else if (node->state == STATE_SHARED){
    /******************** WRITE HIT (SHARED) ********************/
    delete(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, node);
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
    bus_transaction(BUS_WRITE_HIT, i, addr);
    mesi_cache_stat[i].broadcasts += 1;
  } else  {
    /******************** WRITE HIT (EXCLUSIVE, MODIFIED) ********************/
    delete(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, node);
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
  }
  
  //print_cache_list(i);
}

void perform_access_load(int addr, int i)
{
  mesi_cache_stat[i].accesses += 1;
  //print_basic(i, addr);
  int num_shift = LOG2(cache_block_size);
  addr = (addr >> num_shift);
  Pcache_line node = get_node(i, addr);

  /******************** READ MISS (NOT in the cache) ********************/
  if (node == NULL){    

    // increment misses counter
    mesi_cache_stat[i].misses += 1;
    // increment fetch 
    mesi_cache_stat[i].demand_fetches += 1;

    /***** if cache is full, remove the tail, writeback if tail is modified *****/
    if (mesi_cache[i].cache_contents >= cache_usize / cache_block_size){ 
      // increment replacement counter
      mesi_cache_stat[i].replacements += 1;
      // get tail info
      unsigned tail_tag = mesi_cache[i].LRU_tail->tag;
      int tail_state = mesi_cache[i].LRU_tail->state;
      // write back if modified 
      if (tail_state == STATE_MODIFIED){
        /***** WRITE BACK *****/
        // increment writeback counter 
        mesi_cache_stat[i].copies_back += 1; 
      }
      // remove tail 
      remove_tail(i);
      mesi_cache[i].cache_contents -= 1;
    }

    /***** insert new block *****/
    // create a new block 
    Pcache_line new_cache_line = (Pcache_line )malloc(sizeof(cache_line));
    new_cache_line->tag = addr;
    // determine if goes to EXECUTIVE OR SHARED
    if (no_exclusive(i, addr) == TRUE) {
      new_cache_line->state = STATE_EXCLUSIVE;
    } else {
      new_cache_line->state = STATE_SHARED;
    }
    // add new block to cache 
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
    // increment counter 
    mesi_cache[i].cache_contents += 1;

    /***** broadcast READ MISS *****/
    // send BUS READ MISS 
    bus_transaction(BUS_READ_MISS, i, addr);
    // increment broadcasts
    mesi_cache_stat[i].broadcasts += 1;

  } else if (node->state == STATE_INVALID) {
    /******************** READ MISS (INVALID) ********************/
    // increment misses counter
    mesi_cache_stat[i].misses += 1;
    // fetching memory
    mesi_cache_stat[i].demand_fetches += 1;

    /***** replace invalid block *****/ 
    // remove invalid block
    delete(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, node);
    // create a new block  
    Pcache_line new_cache_line = (Pcache_line )malloc(sizeof(cache_line));
    new_cache_line->tag = addr;
    // determine if goes to EXECUTIVE OR SHARED
    if (no_exclusive(i, addr) == TRUE) {
      new_cache_line->state = STATE_EXCLUSIVE;
    } else {
      new_cache_line->state = STATE_SHARED;
    }
    // add new block to cache 
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
    // dont need to increment counter 

    /***** broadcast READ MISS *****/
    // send BUS READ MISS 
    bus_transaction(BUS_READ_MISS, i, addr);
    // increment broadcasts
    mesi_cache_stat[i].broadcasts += 1;

  } else {
    /********** READ HIT (SHARED, EXCLUSIVE, MODIFIED) **********/
    /***** replace old block *****/ 
    // get information of existing block
    unsigned old_tag = node->tag;       
    int old_state = node->state;   
    // delete the old block 
    delete(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, node);
    // make a copy of the block 
    Pcache_line new_cache_line = (Pcache_line )malloc(sizeof(cache_line));
    new_cache_line->tag = old_tag;
    new_cache_line->state = old_state;    // the state is unchanged 
    // reinsert the block to make the LRU work 
    insert(&mesi_cache[i].LRU_head, &mesi_cache[i].LRU_tail, new_cache_line);
    // dont need to increment counter 
  } 
  //print_cache_list(i);
}
