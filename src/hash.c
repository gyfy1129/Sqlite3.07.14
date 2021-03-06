/*
** 2001 September 22
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This is the implementation of generic hash-tables
** used in SQLite.
*/
/*SQLite中哈希表的实现*/

#include "sqliteInt.h"
#include <assert.h>

/* Turn bulk memory into a hash table object by initializing the
** fields of the Hash structure.
**
** "pNew" is a pointer to the hash table that is to be initialized.
*/

/*
把内存变成哈希表的一个对象去初始化哈希表的结构体
pNew是指针初始化的哈希表。
*/

void sqlite3HashInit(Hash *pNew){/*hash_table initliaze *///Elaine debug 
  assert( pNew!=0 );/*如果括号中的值为0，就跳出；如果为1，继续执行*/
  pNew->first = 0;/*initialize the pointer of pNew *///Elaine debug
  pNew->count = 0;
  pNew->htsize = 0;
  pNew->ht = 0;
}

/* Remove all entries from a hash table.  Reclaim all memory.
** Call this routine to delete a hash table or to reset a hash table
** to the empty state.
*/

/*
删除哈希表中的所有条目，回收内存。使用这个方法删除哈希表或者重置哈希表为空状态。
*/

void sqlite3HashClear(Hash *pH){
  HashElem *elem;         /* For looping over all elements of the table *//*循环表中的所有元素*/

  assert( pH!=0 );/*判读PH是否为0,如果为0,则说明HASH为空，程序会中止，否则说明HASH不为空继续执行*/
  elem = pH->first;
  pH->first = 0;
  sqlite3_free(pH->ht);/*释放HASH函数的ht*/
  pH->ht = 0;//使哈希表的ht为0
  pH->htsize = 0;/*htsize is unsigned int,*///Elaine debug
  while( elem ){//判断elem是否还存在，如果存在继续做释放的动作
    HashElem *next_elem = elem->next;
    sqlite3_free(elem);//释放elem
    elem = next_elem;//elam等于他的下一个elem，依次后移
  }
  pH->count = 0;//表中的count置为0
}

/*
** The hashing function.
*/
/*散列函数*/
static unsigned int strHash(const char *z, int nKey){
  int h = 0;
  assert( nKey>=0 );//如果nKey不大于0，则程序终止，如果大于0，则程序继续执行.
  while( nKey > 0  ){//循环nKey的值，当等于0时，跳出循环.
    h = (h<<3) ^ h ^ sqlite3UpperToLower[(unsigned char)*z++];
    nKey--;//nKEY=nKey-1
  }
  return h;//返回h的数值
}


/* Link pNew element into the hash table pH.  If pEntry!=0 then also
** insert pNew into the pEntry hash bucket.
*/

/*
 把pNew连接到哈希表pH上，如果pEntry不等于0 则插入语pNew到结构体
*/
static void insertElement(
  Hash *pH,              /* The complete hash table *///完整的哈希表
  struct _ht *pEntry,    /* The entry into which pNew is inserted */
  HashElem *pNew         /* The element to be inserted *///被插入的元素
){
  HashElem *pHead;       /* First element already in pEntry */
  if( pEntry ){/*determine the pEntry is empty,*///Elaine debug
    pHead = pEntry->count ? pEntry->chain : 0;/*如果pEntry不空，则为pEntry结构赋值*/
    pEntry->count++;
    pEntry->chain = pNew;
  }else{
    pHead = 0;//如果pEntry为0，则pHead为0
  }
  if( pHead ){//判断pHead的值
    pNew->next = pHead;//pNew的next指向pHead
    pNew->prev = pHead->prev;//pNew的prev指向pHead的prev
    if( pHead->prev ){ pHead->prev->next = pNew; }//pHead的prev的next指向pNew
    else             { pH->first = pNew; }
    pHead->prev = pNew;
  }else{
    pNew->next = pH->first;//如果pHead为0，则pNew的下一个指向pH的first
    if( pH->first ){ pH->first->prev = pNew; }//pH的first的prev指向pNew  
    pNew->prev = 0;
    pH->first = pNew;
  }
}


/* Resize the hash table so that it cantains "new_size" buckets.
**
** The hash table might fail to resize if sqlite3_malloc() fails or
** if the new size is the same as the prior size.
** Return TRUE if the resize occurs and false if not.
*/

/*
调整哈希表的大小，使它可以包含new_size
如果哈希表申请空间失败或者新申请的大小和之前是一样的，哈希表重新改变大小是失败的。
如果重新改变大小发生或者失败没有发生，返回true
*/
static int rehash(Hash *pH, unsigned int new_size){
  struct _ht *new_ht;            /* The new hash table */
  HashElem *elem, *next_elem;    /* For looping over existing elements */

#if SQLITE_MALLOC_SOFT_LIMIT>0 //如果定义的SQLITE_MALLOC_SOFT_LIMIT大于0，则执行下面的if语句，否则不执行
  if( new_size*sizeof(struct _ht)>SQLITE_MALLOC_SOFT_LIMIT ){
    new_size = SQLITE_MALLOC_SOFT_LIMIT/sizeof(struct _ht);
  }
  if( new_size==pH->htsize ) return 0;
#endif

  /* The inability to allocates space for a larger hash table is
  ** a performance hit but it is not a fatal error.  So mark the
  ** allocation as a benign. Use sqlite3Malloc()/memset(0) instead of 
  ** sqlite3MallocZero() to make the allocation, as sqlite3MallocZero()
  ** only zeroes the requested number of bytes whereas this module will
  ** use the actual amount of space allocated for the hash table (which
  ** may be larger than the requested amount).
  */
  
   /*
  无法分配一个较大的哈希表空间是一个性能损失，但这不是一个致命的错误。所以标记分配作为一种有利的。
  使用sqlite3Malloc()/memset(0)而不是sqlite3malloczero()进行分配，sqlite3malloczero()是零请
  求的字节数，这个模块将实际使用量的空间分配哈希表（可大于所要求的数量）。   
  */
  
  sqlite3BeginBenignMalloc();//在src/fault中有这个函数的函数体，必须与sqlite3EndBenignMalloc()配合使用
  new_ht = (struct _ht *)sqlite3Malloc( new_size*sizeof(struct _ht) );//申请新的大小
  sqlite3EndBenignMalloc();

  if( new_ht==0 ) return 0;
  sqlite3_free(pH->ht);
  pH->ht = new_ht;
  pH->htsize = new_size = sqlite3MallocSize(new_ht)/sizeof(struct _ht);/*htsize的大小等于申请空间大小/ht结构体所占字节大小*/
  memset(new_ht, 0, new_size*sizeof(struct _ht));/*将新开辟空间new_ht设置为0*/
  for(elem=pH->first, pH->first=0; elem; elem = next_elem){
    unsigned int h = strHash(elem->pKey, elem->nKey) % new_size;
    next_elem = elem->next;
    insertElement(pH, &new_ht[h], elem);/*把elem连接到PH的哈希表的后面*/
  }
  return 1;
}

/* This function (for internal use only) locates an element in an
** hash table that matches the given key.  The hash for this key has
** already been computed and is passed as the 4th parameter.
*/

/*这个函数（仅供内部使用）位于一个元素在一个哈希表相匹配的给定的键值。
这个键值已被计算并作为第四个参数传递。*/

static HashElem *findElementGivenHash(
  const Hash *pH,     /* The pH to be searched *//*被搜索的哈希表*/
  const char *pKey,   /* The key we are searching for *//*被查找的键值*/
  int nKey,           /* Bytes in key (not counting zero terminator) */
  unsigned int h      /* The hash for this key. */
){
  HashElem *elem;                /* Used to loop thru the element list */
  int count;                     /* Number of elements left to test */

  if( pH->ht ){
    struct _ht *pEntry = &pH->ht[h];
    elem = pEntry->chain;
    count = pEntry->count;
  }else{
    elem = pH->first;
    count = pH->count;
  }
  while( count-- && ALWAYS(elem) ){
    if( elem->nKey==nKey && sqlite3StrNICmp(elem->pKey,pKey,nKey)==0 ){ 
      return elem;
    }
    elem = elem->next;
  }
  return 0;
}

/* Remove a single entry from the hash table given a pointer to that
** element and a hash on the element's key.
*/

/*按照给定的指针和哈希表的元素值删除一个条目*/
static void removeElementGivenHash(
  Hash *pH,         /* The pH containing "elem" */
  HashElem* elem,   /* The element to be removed from the pH */
  unsigned int h    /* Hash value for the element */
){
  struct _ht *pEntry;
  if( elem->prev ){
    elem->prev->next = elem->next; 
  }else{
    pH->first = elem->next;
  }
  if( elem->next ){
    elem->next->prev = elem->prev;
  }
  if( pH->ht ){
    pEntry = &pH->ht[h];
    if( pEntry->chain==elem ){
      pEntry->chain = elem->next;
    }
    pEntry->count--;
    assert( pEntry->count>=0 );
  }
  sqlite3_free( elem );
  pH->count--;
  if( pH->count<=0 ){
    assert( pH->first==0 );/*pH->first为真，继续执行，否则推出执行*/ 
    assert( pH->count==0 );/*pH->count为真，继续执行，否则推出执行*/
    sqlite3HashClear(pH);/*清空PH哈希表中的值*/
  }
}

/* Attempt to locate an element of the hash table pH with a key
** that matches pKey,nKey.  Return the data for this element if it is
** found, or NULL if there is no match.
*/

/*
  在哈希表PH中找到与pkey，nkey匹配的key。
  如果找到返回元素的数据，如果没有找到返回NULL
*/

void *sqlite3HashFind(const Hash *pH, const char *pKey, int nKey){
  HashElem *elem;    /* The element that matches key *//*匹配的元素*/
  unsigned int h;    /* A hash on key */

  assert( pH!=0 );/*pH!=0为真，继续执行，否则推出执行*/
  assert( pKey!=0 );/*pKey!=0为真，继续执行，否则推出执行*/
  assert( nKey>=0 );/*nKey!=0为真，继续执行，否则推出执行*/
  if( pH->ht ){
    h = strHash(pKey, nKey) % pH->htsize;
  }else{
    h = 0;
  }
  elem = findElementGivenHash(pH, pKey, nKey, h);
  return elem ? elem->data : 0;
}

/* Insert an element into the hash table pH.  The key is pKey,nKey
** and the data is "data".
**
** If no element exists with a matching key, then a new
** element is created and NULL is returned.
**
** If another element already exists with the same key, then the
** new data replaces the old data and the old data is returned.
** The key is not copied in this instance.  If a malloc fails, then
** the new data is returned and the hash table is unchanged.
**
** If the "data" parameter to this function is NULL, then the
** element corresponding to "key" is removed from the hash table.
*/

/*
插入元素到哈希表PH中，pkey是键值，nkey和data是数据。
如果哈希表中不存在相同键值的入口项，则插入，然后返回 NULL。
如果存在相同的入口项，然后替换原来的数据并且返回原数据。
键值不在这个实例中。如果申请空间失败，则返回新数据并且哈希表不变。
如果data的参数对于这个函数来说是空的，那么和key对应的元素就要从哈希表中移除

*/

void *sqlite3HashInsert(Hash *pH, const char *pKey, int nKey, void *data){
  unsigned int h;       /* the hash of the key modulo hash table size */
  HashElem *elem;       /* Used to loop thru the element list */
  HashElem *new_elem;   /* New element added to the pH */

  assert( pH!=0 );/*pH!=0为真，继续执行，否则推出执行*/ 
  assert( pKey!=0 );/*pKey!=0为真，继续执行，否则推出执行*/ 
  assert( nKey>=0 );/*nKey!=0为真，继续执行，否则推出执行*/ 
  if( pH->htsize ){
    h = strHash(pKey, nKey) % pH->htsize;/*如果pH的htsize成员变量不为0,即使用哈希表存放数据项那么使用 strHash 函数计算桶号h*/
  }else{
    h = 0;/*如果pH的htsize成员变量为0,那么h=0*/
  }
  elem = findElementGivenHash(pH,pKey,nKey,h);
  if( elem ){
    void *old_data = elem->data;
    if( data==0 ){
      removeElementGivenHash(pH,elem,h);
    }else{
      elem->data = data;
      elem->pKey = pKey;
      assert(nKey==elem->nKey);
    }
    return old_data;
  }
  if( data==0 ) return 0;
  new_elem = (HashElem*)sqlite3Malloc( sizeof(HashElem) );
  if( new_elem==0 ) return data;
  new_elem->pKey = pKey;
  new_elem->nKey = nKey;
  new_elem->data = data;
  pH->count++;
  if( pH->count>=10 && pH->count > 2*pH->htsize ){
    if( rehash(pH, pH->count*2) ){
      assert( pH->htsize>0 );
      h = strHash(pKey, nKey) % pH->htsize;
    }
  }
  if( pH->ht ){
    insertElement(pH, &pH->ht[h], new_elem);
  }else{
    insertElement(pH, 0, new_elem);
  }
  return 0;
}
